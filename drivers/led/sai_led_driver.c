#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/err.h>

struct sai_led_data {
    struct gpio_desc *gpiod;
};

static int sai_led_probe(struct platform_device *pdev)
{
    struct sai_led_data *data;
    const char *state;

    pr_info("sai_led: probe called\n");

    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* Read GPIO from DT */
    data->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(data->gpiod)) {
        pr_err("sai_led: failed to get gpio from DT\n");
        return PTR_ERR(data->gpiod);
    }

    /* Optional property: default-state */
    if (!of_property_read_string(pdev->dev.of_node, "default-state", &state)) {
        if (!strcmp(state, "on"))
            gpiod_set_value(data->gpiod, 1);
        else
            gpiod_set_value(data->gpiod, 0);

        pr_info("sai_led: default-state=%s\n", state);
    }

    platform_set_drvdata(pdev, data);

    pr_info("sai_led: probe success\n");
    return 0;
}

static void sai_led_remove(struct platform_device *pdev)
{
    struct sai_led_data *data = platform_get_drvdata(pdev);

    if (data && data->gpiod)
        gpiod_set_value(data->gpiod, 0);
    pr_info("sai_led: remove called\n");
}

static const struct of_device_id sai_led_of_match[] = {
    { .compatible = "sai,my-led" },
    {},
};
MODULE_DEVICE_TABLE(of, sai_led_of_match);

static struct platform_driver sai_led_driver = {
    .probe  = sai_led_probe,
    .remove = sai_led_remove,
    .driver = {
        .name = "sai_led_driver",
        .of_match_table = sai_led_of_match,
    },
};

module_platform_driver(sai_led_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("Simple LED driver using DT + platform driver");
