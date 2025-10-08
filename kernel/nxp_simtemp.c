#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>          // For of_device_id and Device Tree support
#include <linux/printk.h>      // For pr_info()
#include <linux/slab.h>        // For kzalloc()

static int simtemp_probe(struct platform_device *pdev)
{
    int ret;
    u32 sampling_ms;
    u64 threshold_mC;

    pr_info("simtemp: Probing for simtemp device...\n");

    /* Read the 'sampling-ms' property from the device tree. */
    ret = of_property_read_u32(pdev->dev.of_node, "sampling-ms", &sampling_ms);
    if (ret) {
        pr_err("simtemp: Failed to read 'sampling-ms' property\n");
        return ret;
    }

    /* Read the 'threshold-mC' property from the device tree. */
    ret = of_property_read_u64(pdev->dev.of_node, "threshold-mC", &threshold_mC);
    if (ret) {
        pr_err("simtemp: Failed to read 'threshold-mC' property\n");
        return ret;
    }

    pr_info("simtemp: Found device '%s'\n", pdev->name);
    pr_info("simtemp: Read properties: sampling-ms=%u, threshold-mC=%llu\n", sampling_ms, threshold_mC);
    pr_info("simtemp: Device successfully probed!\n");

    return 0;
}

static int simtemp_remove(struct platform_device *pdev)
{
    pr_info("simtemp: Removing simtemp device.\n");
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
