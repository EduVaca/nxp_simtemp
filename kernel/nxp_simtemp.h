/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nxp_simtemp.h - Header file for a kernel mode driver simulating a
 *                 temperature sensor.
 *
 * Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>
 *
 * See README.md for more information.
 */

#include <linux/types.h>

/*
 * This structure is defined to store the temperature sample.
 */
struct simtemp_sample {
    __u64 timestamp_ns;   // monotonic timestamp
    __u32 temp_mC;        // milli-degree Celsius (e.g., 44123 = 44.123 °C)
    __u16 flags;          // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
    __u16 padding;
} __attribute__((packed));

/* Mode definitions */
enum {
    MODE_NORMAL,
    MODE_RAMP
};

/* Flags for struct simtemp_sample */
#define NEW_SAMPLE         (1 << 0)
#define THRESHOLD_CROSSED  (1 << 1)

#define MIN_SAMPLE_MS  10              // Minimun time in ms for sampling
#define RAMP_START     10              // Low limit before start crossing
                                       // the threshold
#define RAMP_STOP      RAMP_START + 5  // Upper limit before restart the
                                       // crossing threshold
#define KFIFO_SIZE     256             // Number of samples

#define DEFAULT_SAMPLE_MS      100     // Default sampling time
#define DEFAULT_THRESHOLD_MC   45000   // Default milli-degree threshold

/* Device specific parameters */
#define DRIVER_NAME       "simtemp"
#define PLATFORM_DEV_NAME DRIVER_NAME
#define DEVICE_NODE       DRIVER_NAME

#define DEVICE_FILE "/dev/"DEVICE_NODE
#define DEVICE_PATH "/sys/devices/platform/"PLATFORM_DEV_NAME

#ifdef __KERNEL__
/*
 * Structure to hold device-specific data.
 */
struct simtemp_dev {
    struct miscdevice *miscdev;
    struct hrtimer temp_hrtimer;
    wait_queue_head_t read_wait;
    wait_queue_head_t poll_wait;
    DECLARE_KFIFO(kfifo, struct simtemp_sample, KFIFO_SIZE);
    spinlock_t lock; /* Protects access to kfifo */
    struct device *dev;

    u32 sampling_ms;
    u32 threshold_mC;
    u32 mode;
    u32 current_temp;
    u16 current_flags;

    u64 samples_taken;
    u64 threshold_alerts;

    u32 counter;
};
#endif