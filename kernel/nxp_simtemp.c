/* SPDX-License-Identifier: GPL-2.0 */
/*
 * nxp_simtemp.c - Source code for a kernel mode driver simulating a
 *                 temperature sensor.
 *
 * Copyright (c) 2025 Eduardo Vaca <edu.daniel.vs@gmail.com>
 *
 * See README.md for more information.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/sysfs.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/of.h>               // For of_device_id and Device Tree support
#include <linux/slab.h>             // For kzalloc()
#include <linux/workqueue.h>        // For workqueue functions
#include <linux/timekeeping.h>      // For ktime_get_ns()
#include <linux/random.h>           // For get_random_u32()
#include <linux/property.h>         // For struct property_entry
#include <linux/hrtimer.h>
#include <linux/ktime.h>

/* NXP defined structs */
#include "nxp_simtemp.h"
#include "nxp_simtemp_ioctl.h"

/**
 * @note **Version History:**
 *
 * -----------------------------------------------------------------------------
 * ## - 2025-10-14 - 0.1.3
 * ### Enh
 * - Add operation modes; normal: samples are always below the threshold
 *   ramp: samples are below the threshold RAMP_START times and above the
 *   threshold (RAMP_START - RAMP_STOP) times.
 * - Driver parameters can be changed via IOCTL calls.
 *
 * -----------------------------------------------------------------------------
 * ## - 2025-10-14 - 0.1.2
 * ### Enh
 * - Drop the oldest and save the newest when FIFO is full.
 * ### Fixed
 * - Wake up pollers in every sample.
 *
 * -----------------------------------------------------------------------------
 * ## - 2025-10-14 - 0.1.1
 * ### Fixed
 * - Take nanoseconds from ktime_get_real_ns to convert it to UTC in user-space
 *   application.
 *
 * -----------------------------------------------------------------------------
 * ## - 2025-10-14 - 0.1.0
 * ### Pre-Initial Release
 * - Kernel mode driver binds to "simtemp" device.
 * - Register a misc device with fs attributes and char device with file ops
 * - Simulate temperature samples using a High-Resolution timer.
 * - Store samples and metada in a KFIFO
 * - Add platform device if binding to DTB fails.
 *
 * -----------------------------------------------------------------------------
 */
#define DRIVER_VERSION "0.1.3"

/* Device state holder */
static struct simtemp_dev *simtemp_data;

/* --- Sysfs Attributes --- */
static ssize_t sampling_ms_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct simtemp_dev *sdev = dev->driver_data;
    unsigned long flags;
    u32 sampling_ms;

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    sampling_ms = sdev->sampling_ms;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */
    
    return scnprintf(buf, PAGE_SIZE, "%u\n", sampling_ms);
}

static ssize_t sampling_ms_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct simtemp_dev *sdev = dev->driver_data;
    u32 val;
    int err;
    unsigned long flags;

    err = kstrtou32(buf, 10, &val);
    if (err) {
        return err;
    }
    if (val < MIN_SAMPLE_MS) { /* Minimum MIN_SAMPLE_MS for stability */
        return -EINVAL;
    }

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    sdev->sampling_ms = val;
    hrtimer_cancel(&sdev->temp_hrtimer);
    hrtimer_start(&sdev->temp_hrtimer, ms_to_ktime(sdev->sampling_ms),
        HRTIMER_MODE_REL);
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return count;
}
static DEVICE_ATTR_RW(sampling_ms);

static ssize_t threshold_mC_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct simtemp_dev *sdev = dev->driver_data;
    u32 threshold_mC;
    unsigned long flags;

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    threshold_mC = sdev->threshold_mC;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return scnprintf(buf, PAGE_SIZE, "%u\n", threshold_mC);
}

static ssize_t threshold_mC_store(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct simtemp_dev *sdev = dev->driver_data;
    u32 val;
    int err;
    unsigned long flags;

    err = kstrtos32(buf, 10, &val);
    if (err) {
        return err;
    }

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    sdev->threshold_mC = val;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return count;
}
static DEVICE_ATTR_RW(threshold_mC);

static ssize_t mode_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct simtemp_dev *sdev = dev->driver_data;
    unsigned long flags;
    const char *mode_str;
    u32 mode;

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    mode = simtemp_data->mode;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    switch (mode) {
        case MODE_NORMAL:
            mode_str = "normal";
            break;
        case MODE_RAMP:
            mode_str = "ramp";
            break;
        default:
            mode_str = "unknown";
            break;
    }

    return scnprintf(buf, PAGE_SIZE, "%s\n", mode_str);
}

static ssize_t mode_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    struct simtemp_dev *sdev = dev->driver_data;
    unsigned long flags;
    u32 new_mode;

    if (sysfs_streq(buf, "normal")) {
        new_mode = MODE_NORMAL;
    } else if (sysfs_streq(buf, "ramp")) {
        new_mode = MODE_RAMP;
    } else {
        return -EINVAL;
    }

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    simtemp_data->mode = new_mode;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t stats_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct simtemp_dev *sdev = dev->driver_data;
    u64 samples_taken, threshold_alerts;
    unsigned long flags;

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    samples_taken = sdev->samples_taken;
    threshold_alerts = sdev->threshold_alerts;
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return scnprintf(buf, PAGE_SIZE, "samples_taken: %llu\nthreshold_alerts:"
        " %llu\n", samples_taken, threshold_alerts);
}
static DEVICE_ATTR_RO(stats);

static struct attribute *simtemp_attrs[] = {
    &dev_attr_sampling_ms.attr,
    &dev_attr_threshold_mC.attr,
    &dev_attr_mode.attr,
    &dev_attr_stats.attr,
    NULL,
};

static const struct attribute_group simtemp_group = {
    .attrs = simtemp_attrs,
};

/* --- Char Device File Operations --- */
static int simtemp_open(struct inode *inode, struct file *file) {
    file->private_data = simtemp_data;
    dev_info(simtemp_data->dev, "Device opened.\n");
    return 0;
}

static int simtemp_release(struct inode *inode, struct file *file) {
    struct simtemp_dev *sdev = file->private_data;
    dev_info(sdev->dev, "Device released.\n");
    return 0;
}

static ssize_t simtemp_read(struct file *file, char __user *buf, size_t count,
    loff_t *ppos) {
    struct simtemp_dev *sdev = file->private_data;
    struct simtemp_sample sample;
    int ret;
    int copied;
    unsigned long flags;


    if (count < sizeof(sample)) {
        return -EINVAL;
    }

    if (kfifo_is_empty(&sdev->kfifo)) {
        if (file->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }

        /* Wait for data with exclusive wake-up */
        ret = wait_event_interruptible_exclusive(sdev->read_wait,
            kfifo_is_empty(&sdev->kfifo) == 0);
        if (ret) {
            return ret; // Signal received
        }
    }

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    ret = kfifo_get(&sdev->kfifo, &sample);
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */
    if (ret == 0) { /* Should not happen if wait_event worked */
        return -EAGAIN;
    }

    copied = min_t(size_t, count, sizeof(sample));
    if (copy_to_user(buf, &sample, copied)) {
        return -EFAULT;
    }

    return copied;
}

static __poll_t simtemp_poll(struct file *file,
    struct poll_table_struct *wait) {
    __poll_t mask = 0;
    struct simtemp_dev *sdev = file->private_data;
    unsigned long flags;

    poll_wait(file, &sdev->poll_wait, wait);

    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);
    if (!kfifo_is_empty(&sdev->kfifo)) {
        mask |= POLLIN | POLLRDNORM;
    }
    if (sdev->current_flags & THRESHOLD_CROSSED) {
        mask |= POLLPRI;
    }
    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    return mask;
}

static long simtemp_ioctl(struct file *file, unsigned int cmd,
    unsigned long arg)
{
    struct simtemp_dev *sdev = file->private_data;
    struct simtemp_config cfg;
    int err = 0;
    unsigned long flags;


    if (_IOC_TYPE(cmd) != SIMTEMP_IOC_MAGIC)
        return -ENOTTY;

    switch (cmd) {
        case SIMTEMP_IOC_SET_ALL:
            if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg))) {
                return -EFAULT;
            }

            /* START CRITICAL BLOCK */
            spin_lock_irqsave(&sdev->lock, flags);
            sdev->sampling_ms = cfg.sampling_ms;
            sdev->threshold_mC = cfg.threshold_mC;
            sdev->mode = cfg.mode;
            hrtimer_cancel(&sdev->temp_hrtimer);
            hrtimer_start(&sdev->temp_hrtimer, ms_to_ktime(sdev->sampling_ms),
                HRTIMER_MODE_REL);
            spin_unlock_irqrestore(&sdev->lock, flags);
            /* END CRITICAL BLOCK */
            dev_info(sdev->dev, "Config updated via ioctl.\n");
            break;
        default:
            err = -ENOTTY;
            break;
    }

    return err;
}

static const struct file_operations simtemp_fops = {
    .owner          = THIS_MODULE,
    .open           = simtemp_open,
    .release        = simtemp_release,
    .read           = simtemp_read,
    .poll           = simtemp_poll,
    .unlocked_ioctl = simtemp_ioctl,
};

static struct miscdevice misc_simtemp_dev = {
    .minor      = TEMP_MINOR,
    .name       = DEVICE_NODE,
    .fops       = &simtemp_fops,
};

/**
 * @brief Get a random value as the current temperature.
 * @param sdev Pointer to simtemp_dev.
 * @return A random temperature value from 0 to threshold_mC,
 *         and threshold_mC + 1 every MAX_COUNT
 */
static u32 get_temperature (struct simtemp_dev *sdev) {
    u32 rand_val, temp;

    /* Generate a new simulated temperature value */
    rand_val = get_random_u32();
    temp = rand_val % sdev->threshold_mC;
    sdev->counter += 1;

    switch (sdev->mode) {
        case MODE_RAMP:
            /* Simulate a threshold crossed read every MAX_COUNT samples */
            if (sdev->counter > RAMP_START) {
                temp = sdev->threshold_mC + sdev->counter;
                if (sdev->counter >= RAMP_STOP) {
                    sdev->counter = 0;
                }
            }
            break;
    }

    return temp;
}

/**
 * @brief High-resolution timer function that will be executed periodically.
 * @param timer Pointer to hrtimer struct.
 * @return A timer restart value HRTIMER_RESTART
 */
static enum hrtimer_restart simtemp_hrtimer_callback(struct hrtimer *timer) {
    struct simtemp_dev *sdev = container_of(timer, struct simtemp_dev,
        temp_hrtimer);
    struct simtemp_sample sample, drop_sample;
    __poll_t mask = 0;
    u16 old_flags;
    unsigned long flags;
    int ret;


    /* START CRITICAL BLOCK */
    spin_lock_irqsave(&sdev->lock, flags);

    old_flags = sdev->current_flags;

    /* Update global fields */
    sdev->current_temp = get_temperature(sdev);
    sdev->current_flags |= NEW_SAMPLE;
    sdev->samples_taken++;

    /* Get the current real time in nanoseconds since the Unix epoch */
    sample.timestamp_ns = ktime_get_real_ns();
    sample.temp_mC = sdev->current_temp;

    if (sdev->current_temp >= sdev->threshold_mC) {
        sdev->current_flags |= THRESHOLD_CROSSED;
        sdev->threshold_alerts++;
        /* Set flag to wake up pollers for urgent data (threshold crossing) */
        mask |= POLLPRI;
        dev_info(sdev->dev, "Threshold crossed! temp=%u mC, threshold=%u mC\n",
                 sdev->current_temp, sdev->threshold_mC);

    } else { /* Clean flags */
        sdev->current_flags &= ~THRESHOLD_CROSSED;
    }

    sample.flags = sdev->current_flags;

    /* Update FIFO */
    if (kfifo_is_full(&sdev->kfifo)) {
        ret = kfifo_get(&sdev->kfifo, &drop_sample);
        dev_warn(sdev->dev, "kfifo is full, dropping latest sample: %u mC at"
            " %llu ns, flags=0x%02x\n, ret=%d", drop_sample.temp_mC,
            drop_sample.timestamp_ns, drop_sample.flags, ret);
    }

    /* Should be at least one slot for this sample*/
    kfifo_put(&sdev->kfifo, sample);
    /* Wake up ONE blocking reader */
    wake_up_interruptible(&sdev->read_wait);
    /* Wake up pollers for new data */
    wake_up_interruptible_poll(&sdev->poll_wait, (mask | POLLIN));

    dev_info(sdev->dev, "New sample recorded: %u mC at %llu ns, flags=0x%02x\n",
            sample.temp_mC, sample.timestamp_ns, sample.flags);

    spin_unlock_irqrestore(&sdev->lock, flags);
    /* END CRITICAL BLOCK */

    /* Restart the timer */
    hrtimer_forward_now(timer, ms_to_ktime(sdev->sampling_ms));
    return HRTIMER_RESTART;
}

/* --- Platform Driver Core --- */
static int simtemp_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;

    dev_info(dev, "Probing for simtemp device...\n");

    simtemp_data = devm_kzalloc(dev, sizeof(*simtemp_data), GFP_KERNEL);
    if (!simtemp_data) {
        return -ENOMEM;
    }

    /*
     * Link the data to the device so it gets remove automatically
     * when the devie is remove.
     */
    platform_set_drvdata(pdev, simtemp_data);

    /* Read the 'sampling-ms' property from the device tree. */
    ret = device_property_read_u32(dev, "sampling-ms",
        &simtemp_data->sampling_ms);
    if (ret) {
        dev_err(dev, "Failed to read 'sampling-ms' property\n");
        return ret;
    }

    /* Read the 'threshold-mC' property from the device tree. */
    ret = device_property_read_u32(dev, "threshold-mC",
        &simtemp_data->threshold_mC);
    if (ret) {
        dev_err(dev, "Failed to read 'threshold-mC' property\n");
        return ret;
    }

    dev_info(dev, "Device parameters: sampling-ms=%u, threshold-mC=%u\n",
             simtemp_data->sampling_ms, simtemp_data->threshold_mC);

    /* Generate a new simulated temperature value. */
    simtemp_data->current_temp = get_temperature(simtemp_data);
    simtemp_data->mode = MODE_NORMAL;

    /* Initialize wait queues for read/epoll/select operations. */
    init_waitqueue_head(&simtemp_data->read_wait);
    init_waitqueue_head(&simtemp_data->poll_wait);

    /* Initialize spinlock for protecting the sample data. */
    spin_lock_init(&simtemp_data->lock);

    /* Allocates and initializes dynamically. */
    INIT_KFIFO(simtemp_data->kfifo);

    /* Initialize a high-resolution timer for simulated samples. */
    hrtimer_init(&simtemp_data->temp_hrtimer,
        CLOCK_MONOTONIC, HRTIMER_MODE_REL);

    /* Set the callback function. */
    simtemp_data->temp_hrtimer.function = &simtemp_hrtimer_callback;

    /* Save a reference to the device. */
    simtemp_data->dev = dev;

    /* Start the timer. */
    hrtimer_start(&simtemp_data->temp_hrtimer,
        ms_to_ktime(simtemp_data->sampling_ms), HRTIMER_MODE_REL);

    ret = misc_register(&misc_simtemp_dev);
    if (ret) {
        dev_err(dev, "Failed to register miscdevice.\n");
        hrtimer_cancel(&simtemp_data->temp_hrtimer);
        return ret;
    }
    simtemp_data->miscdev = &misc_simtemp_dev;

    ret = sysfs_create_group(&pdev->dev.kobj, &simtemp_group);
    if (ret) {
        dev_err(dev, "Failed to create sysfs attributes.\n");
        misc_deregister(simtemp_data->miscdev);
        hrtimer_cancel(&simtemp_data->temp_hrtimer);
        return ret;
    }

    dev_info(dev, "Found device '%s'\n", pdev->name);
    dev_info(dev, "Device registered as /dev/%s\n", pdev->name);
    dev_info(dev, "Read properties: sampling-ms=%u, threshold-mC=%u\n",
        simtemp_data->sampling_ms, simtemp_data->threshold_mC);
    dev_info(dev, "Device successfully probed!\n");

    return 0;
}

#if defined(RBPITGT)
static int simtemp_remove(struct platform_device *pdev)
#else
static void simtemp_remove(struct platform_device *pdev)
#endif
{
    struct simtemp_dev *sdev;
    sdev = platform_get_drvdata(pdev);
    dev_info(sdev->dev, "Removing simtemp device.\n");

    /* Remove the corresponding sysfs entries. */
    sysfs_remove_group(&pdev->dev.kobj, &simtemp_group);

    /* De-register the device and free its spot. */
    misc_deregister(&misc_simtemp_dev);

    /* Stop the high-resolution timer before exiting. */
    hrtimer_cancel(&sdev->temp_hrtimer);

#if defined(RBPITGT)
    return 0;
#endif
}

/* Match the device tree compatible string to this driver. */
static const struct of_device_id simtemp_of_match[] = {
    { .compatible = "nxp,"PLATFORM_DEV_NAME, },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, simtemp_of_match);

/* Platform DRIVER structure. */
static struct platform_driver simtemp_driver = {
    .probe  = simtemp_probe,
    .remove = simtemp_remove,
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = simtemp_of_match,
    },
};


/*
 * Variables to hold device/driver states.
 *
 * This is needed for testing purposes so a local platform device is bind to
 * this device driver in absent of a DT's device node
 */
static bool platform_driver_registered;
static bool platform_device_registered;
static struct platform_device *simtemp_device_simple;

/*
 * Define integer values as device properties as is common device tree bindings.
 */
static const u32 prop_sampling_ms = DEFAULT_SAMPLE_MS;
static const u32 prop_threshold_mC = DEFAULT_THRESHOLD_MC;

/* Device properties */
static struct property_entry simtemp_properties[] = {
    PROPERTY_ENTRY_U32("sampling-ms", prop_sampling_ms),
    PROPERTY_ENTRY_U32("threshold-mC", prop_threshold_mC),
    { /* sentinel */ },
};

/* Platform DEVICE structure */
static struct platform_device_info simtemp_device = {
    .name           = PLATFORM_DEV_NAME,    // Device name for driver matching
    .id             = PLATFORM_DEVID_NONE,  // Instance ID
    .properties     = simtemp_properties,   // Attach our properties
};

/**
 * @brief Entry point for device driver call at insmod.
 * @return 0 if device allocation and proving successed,
 *         different than 0 otherwise.
 */
static int __init simtemp_init(void) {
    int retval;

    pr_info(DRIVER_NAME": Entry point\n");

    /* Try to bind the device registered in the device tree blob (DTB) */
    retval = platform_driver_probe(&simtemp_driver, simtemp_probe);
    if (retval == 0) {
        platform_driver_registered = true;
        pr_info(DRIVER_NAME": Successfully added platform device %s\n",
            simtemp_driver.driver.name);
    } else {
        pr_err(DRIVER_NAME": Failed to bind driver %s to %s: %d\n",
            simtemp_driver.driver.name, PLATFORM_DEV_NAME, retval);
    }

    /*
     * If prove failed, then the DTB entry is not available
     * so create a software device for testing purposes
     */
    if (platform_driver_registered == false) {
        simtemp_device_simple = platform_device_register_full(&simtemp_device);
        if (IS_ERR(simtemp_device_simple)) {
            retval = PTR_ERR(simtemp_device_simple);
            pr_err(DRIVER_NAME": Failed to add platform device with properties:"
                "%d\n", retval);
        } else {
            platform_device_registered = true;
            pr_info(DRIVER_NAME": Platform device %s was registered"
                " correctly\n", simtemp_device.name);
        }
    }

    /* If the software device was created, then try to bind again. */
    if (platform_device_registered == true) {
        retval = platform_driver_probe(&simtemp_driver, simtemp_probe);
        if (retval == 0) {
            platform_driver_registered = true;
            pr_info(DRIVER_NAME": Successfully added platform device %s\n",
                simtemp_driver.driver.name);
        } else {
            pr_err(DRIVER_NAME": Failed to bind driver %s to %s: %d\n",
                simtemp_driver.driver.name, PLATFORM_DEV_NAME, retval);
        }
    }

    /* If any errors, release memory/structs allocated. */
    if (retval) {
        if (platform_device_registered) {
            platform_device_unregister(simtemp_device_simple);
        }
        if (platform_driver_registered) {
            platform_driver_unregister(&simtemp_driver);
        }
    }

    return retval;
}

/**
 * @brief Exit point for device driver, release memory/structs allocated.
 */
static void __exit simtemp_exit(void) {

    pr_info(DRIVER_NAME": Exit point\n");

    if (platform_device_registered) {
        platform_device_unregister(simtemp_device_simple);
    }
    if (platform_driver_registered) {
        platform_driver_unregister(&simtemp_driver);
    }
}

/* Register entry/exit points. */
module_init(simtemp_init);
module_exit(simtemp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Eduardo Vaca <edu.daniel.vs@gmail.com>");
MODULE_DESCRIPTION("A dummy platform driver for an NXP simuldated temperature device.");
MODULE_VERSION(DRIVER_VERSION);
