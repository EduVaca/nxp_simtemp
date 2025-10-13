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

/* Flags for struct simtemp_sample */
#define NEW_SAMPLE         (1 << 0)
#define THRESHOLD_CROSSED  (1 << 1)

void ns_to_iso8601(long long ns, char* buffer, size_t size);
void print_help(char *prog_name);
