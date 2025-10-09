#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>               // For of_device_id and Device Tree support
#include <linux/slab.h>             // For kzalloc()
#include <linux/workqueue.h>        // For workqueue functions
#include <linux/timekeeping.h>      // For ktime_get_ns()
#include <linux/random.h>           // For get_random_u32()

#include <linux/hrtimer.h>
#include <linux/ktime.h>

/* NXP defined structs */
#include "nxp_simtemp.h"

/* Use a standard major.minor.patch versioning scheme */
#define DRIVER_VERSION "1.1.0"

/*
 * The high-resolution timer function that will be executed periodically.
 */
static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer) {
    struct simtemp_data *data = container_of(timer, struct simtemp_data, temp_hrtimer);
    u32 rand_val;
    s32 temp;
    unsigned long flags;

    // Generate a new simulated temperature value
    rand_val = get_random_u32();
    temp = rand_val % data->threshold_mC;

    // START CRITICAL BLOCK
    spin_lock_irqsave(&data->lock, flags);

    data->latest_sample.timestamp_ns = ktime_get_ns();
    // Simulate a threshold crossed read every 10 samples
    if (++data->counter > MAX_COUNT) {
        data->counter = 0;
        temp = data->threshold_mC + 1;
    }
    data->latest_sample.temp_mC = temp;
    data->latest_sample.flags = NEW_SAMPLE;

    if (temp >= data->threshold_mC) {
        data->latest_sample.flags |= THRESHOLD_CROSSED;
        dev_info(data->dev, "Threshold crossed! temp=%d mC, threshold=%llu mC\n",
                 temp, data->threshold_mC);
    }

    spin_unlock_irqrestore(&data->lock, flags);
    // END CRITICAL BLOCK

    dev_dbg(data->dev, "New sample recorded: %d mC at %llu ns\n",
            temp, data->latest_sample.timestamp_ns);

    // Restart the timer
    hrtimer_forward_now(timer, ms_to_ktime(data->sampling_ms)); // Use hrtimer_forward_now to restart

    return HRTIMER_RESTART;
}

static int simtemp_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct simtemp_data *data;
    int ret;

    dev_info(dev, "Probing for simtemp device...\n");

    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, data);

    /* Read the 'sampling-ms' property from the device tree. */
    ret = of_property_read_u32(pdev->dev.of_node, "sampling-ms", &data->sampling_ms);
    if (ret) {
        dev_err(dev, "Failed to read 'sampling-ms' property\n");
        return ret;
    }

    /* Read the 'threshold-mC' property from the device tree. */
    ret = of_property_read_u64(pdev->dev.of_node, "threshold-mC", &data->threshold_mC);
    if (ret) {
        dev_err(dev, "Failed to read 'threshold-mC' property\n");
        return ret;
    }

    dev_info(dev, "Device parameters: sampling-ms=%u, threshold-mC=%llu\n",
             data->sampling_ms, data->threshold_mC);

    // Initialize spinlock for protecting the sample data
    spin_lock_init(&data->lock);

    // Initialize a high-resolution timer for simulated samples
    hrtimer_init(&data->temp_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL); // Initialize

    // Set the callback function
    data->temp_hrtimer.function = &my_hrtimer_callback;

    // Start the timer
    hrtimer_start(&data->temp_hrtimer, ms_to_ktime(data->sampling_ms), HRTIMER_MODE_REL);

    // Save a reference to the device
    data->dev = dev;
    dev_info(dev, "Found device '%s'\n", pdev->name);
    dev_info(dev, "Read properties: sampling-ms=%u, threshold-mC=%llu\n", data->sampling_ms, data->threshold_mC);
    dev_info(dev, "Device successfully probed!\n");

    return 0;
}

static int simtemp_remove(struct platform_device *pdev)
{
    struct simtemp_data *data = platform_get_drvdata(pdev);
    dev_info(&pdev->dev, "Removing simtemp device.\n");

    // Stop the high-resolution timer before exiting
    hrtimer_cancel(&data->temp_hrtimer);
    return 0;
}

/* Match the device tree compatible string to this driver. */
static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,simtemp", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

/* Define the platform driver structure. */
static struct platform_driver simtemp_driver = {
    .probe  = simtemp_probe,
    .remove = simtemp_remove,
    .driver = {
        .name           = "simtemp",
        .of_match_table = simtemp_of_match,
    },
};

/* Register the driver at module initialization. */
module_platform_driver(simtemp_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo Vaca");
MODULE_DESCRIPTION("A dummy platform driver for an NXP simuldated temperature device.");
MODULE_VERSION(DRIVER_VERSION);
