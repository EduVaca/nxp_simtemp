/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nxp_simtemp_ioctl.h - Header file for a kernel mode driver simulating a
 *                       temperature sensor.
 *
 * Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>
 *
 * See README.md for more information.
 */

#include <linux/types.h>

/* IOCTL configuration structure */
struct simtemp_config {
    __u32 sampling_ms;
    __u32 threshold_mC;
    __u32 mode;
};

/* IOCTL command definitions */
#define SIMTEMP_IOC_MAGIC 'T'
#define SIMTEMP_IOC_SET_ALL _IOW(SIMTEMP_IOC_MAGIC, 1, struct simtemp_config)
