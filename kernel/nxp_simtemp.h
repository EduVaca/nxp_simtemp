#include <linux/types.h>

/*
 * This structure is defined to store the temperature sample.
 */
struct simtemp_sample {
    __u64 timestamp_ns;   // monotonic timestamp
    __s32 temp_mC;        // milli-degree Celsius (e.g., 44123 = 44.123 °C)
    __u32 flags;          // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
};

/* Flags for struct simtemp_sample */
#define NEW_SAMPLE (1 << 0)
#define THRESHOLD_CROSSED (1 << 1)

#define MAX_COUNT 10

/*
 * This structure holds the device-specific data.
 */
struct simtemp_data {
    u32 sampling_ms;
    u64 threshold_mC;
    u32 counter;
    struct device *dev;
    struct hrtimer temp_hrtimer;
    struct simtemp_sample latest_sample;
    spinlock_t lock; // Protects access to latest_sample
};