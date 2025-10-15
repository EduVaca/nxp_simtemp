/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nxp_simtemp_test.c - Source code for user space testing application to
 *                      validate a kernel mode driver simulating a temperature
*                       sensor.
 *
 * Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>
 *
 * See README.md for more information.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <poll.h>

/* NXP defined structs */
#include "nxp_simtemp.h"
#include "nxp_simtemp_ioctl.h"
#include "nxp_simtemp_test.h"

/**
 * @brief Convert nanoseconds to time in iso8601 format.
 * @param ns Time in nanoseconds.
 * @param buffer Buffer to hold the formated time
 * @param size Size of buffer
 */
void ns_to_iso8601(long long ns, char* buffer, size_t size) {
    time_t sec = ns / 1000000000;
    long long nanosec = ns % 1000000000;
    struct tm *tm_info = gmtime(&sec);

    strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", tm_info);
    snprintf(buffer + strlen(buffer), size - strlen(buffer), ".%03uZ",
        (unsigned int)(nanosec / 1000000));
}

/**
 * @brief Print's program user help.
 * @param prog_name Program name.
 */
void print_help(char *prog_name) {
    fprintf(stderr, "Usage: %s [options]\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -s <ms>           Set sampling period via sysfs.\n");
    fprintf(stderr, "  -t <mC>           Set threshold via sysfs.\n");
    fprintf(stderr, "  -m <mode>         Set mode via sysfs (normal|ramp).\n");
    fprintf(stderr, "  -i <ms>:<mC>:<mode>  Set all via ioctl (mode: 0=normal,"
                                         " 1=ramp).\n");
    fprintf(stderr, "  -p                Run in poll loop, printing samples and"
                                         " alerts.\n");
    exit(EXIT_FAILURE);
}

/**
 * @brief Entry point
 * @param argc Parameters counter.
 * @param argv Parameters values.
 */
int main(int argc, char *argv[]) {
    int fd;
    char path[256];
    struct pollfd pfd;
    int ret;
    char timestamp_str[64];
    struct simtemp_sample sample;
    struct simtemp_config cfg;
    char *token;

    if (argc < 2) {
        print_help(argv[0]);
    }

    if (strcmp(argv[1], "-s") == 0 && argc == 3) {
        snprintf(path, sizeof(path), DEVICE_PATH"/sampling_ms");
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open sysfs"); return 1;
        }
        if (write(fd, argv[2], strlen(argv[2])) < 0) {
            perror("write sysfs"); return 1;
        }
        close(fd);
        printf("Set sampling_ms to %s via sysfs.\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-t") == 0 && argc == 3) {
        snprintf(path, sizeof(path), DEVICE_PATH"/threshold_mC");
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open sysfs"); return 1;
        }
        if (write(fd, argv[2], strlen(argv[2])) < 0) {
            perror("write sysfs"); return 1;
        }
        close(fd);
        printf("Set threshold_mC to %s via sysfs.\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-m") == 0 && argc == 3) {
        snprintf(path, sizeof(path), DEVICE_PATH"/mode");
        fd = open(path, O_WRONLY);
        if (fd < 0) {
            perror("open sysfs");
            return 1;
        }
        if (write(fd, argv[2], strlen(argv[2])) < 0) {
            perror("write sysfs");
            return 1;
        }
        close(fd);
        printf("Set mode to %s via sysfs.\n", argv[2]);
        return 0;
    }

    if (strcmp(argv[1], "-i") == 0 && argc == 3) {
        fd = open(DEVICE_FILE, O_RDONLY);
        if (fd < 0) {
            perror("open device");
            return 1;
        }

        token = strtok(argv[2], ":");
        if (token) {
            cfg.sampling_ms = atoi(token);
        } else {
            fprintf(stderr, "Invalid ioctl config format.\n");
            close(fd);
            return 1;
        }

        token = strtok(NULL, ":");
        if (token) {
            cfg.threshold_mC = atoi(token);
        } else {
            fprintf(stderr, "Invalid ioctl config format.\n");
            close(fd);
            return 1;
        }

        token = strtok(NULL, ":");
        if (token) {
            cfg.mode = atoi(token);
        } else {
            fprintf(stderr, "Invalid ioctl config format.\n");
            close(fd);
            return 1;
        }

        if (ioctl(fd, SIMTEMP_IOC_SET_ALL, &cfg) < 0) {
            perror("ioctl");
            close(fd);
            return 1;
        }
        printf("Set conf via ioctl: sampling_ms=%u, threshold_mC=%u, mode=%u\n",
               cfg.sampling_ms, cfg.threshold_mC, cfg.mode);
        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "-p") == 0) {
        fd = open(DEVICE_FILE, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            perror("open device");
            return 1;
        }

        pfd.fd = fd;
        pfd.events = POLLIN | POLLPRI;

        printf("Polling for samples and alerts. Ctrl+C to exit.\n");

        while (1) {
            ret = poll(&pfd, 1, -1);
            if (ret < 0) {
                perror("poll");
                break;
            }

            if ((pfd.revents & (POLLIN | POLLPRI)) > 0) {
                if (read(fd, &sample, sizeof(sample)) == sizeof(sample)) {
                    ns_to_iso8601(sample.timestamp_ns, timestamp_str,
                        sizeof(timestamp_str));
                    if (pfd.revents & POLLPRI) {
                        printf("%s live alert\n", timestamp_str);
                    }
                    if (sample.flags & THRESHOLD_CROSSED) {
                        printf("%s temp=%.3fC alert=1 (Threshold crossed)\n",
                            timestamp_str, (float)sample.temp_mC / 1000.0);
                    } else {
                        printf("%s temp=%.3fC alert=0\n",
                            timestamp_str, (float)sample.temp_mC / 1000.0);
                    }
                }
            }
        }
        close(fd);
        return 0;
    }

    print_help(argv[0]);
    return 1;
}
