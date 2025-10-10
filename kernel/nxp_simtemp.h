#include <linux/types.h>

/*
 * This structure is defined to store the temperature sample.
 */
struct simtemp_sample {
    u64 timestamp_ns;   // monotonic timestamp
    u32 temp_mC;        // milli-degree Celsius (e.g., 44123 = 44.123 °C)
    u16 flags;          // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
    u16 padding;
} __attribute__((packed));

/* Flags for struct simtemp_sample */
#define NEW_SAMPLE         (1 << 0)
#define THRESHOLD_CROSSED  (1 << 1)

#define MIN_SAMPLE_MS  10     // Minimun time in ms for sampling
#define MAX_COUNT      10     // Samples before simulate a threshold crossed
#define KFIFO_SIZE     256    // Number of samples

/*
 * This structure holds the device-specific data.
 */
struct simtemp_dev {
    struct miscdevice *miscdev;
    struct hrtimer temp_hrtimer;
    wait_queue_head_t read_wait;
    wait_queue_head_t poll_wait;
    DECLARE_KFIFO(kfifo, struct simtemp_sample, KFIFO_SIZE);
    spinlock_t lock; // Protects access to kfifo
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
