// sai_gpio_irq.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define DEVICE_NAME "sai_gpio_irq"

/* BCM 27 = Pin 13 => 512 + 27 = 539 */
/* BCM 17 = Pin 11 => 512 + 17 = 529 */
#define GPIO_IN    539
#define GPIO_OUT   529

static struct gpio_desc *gpio_in;
static struct gpio_desc *gpio_out;
static int irq_line;

/* IRQ handler (threaded) */
static irqreturn_t sai_irq_handler(int irq, void *data)
{
    int state;

    /* Read input */
    state = gpiod_get_value(gpio_in);

    /* Toggle LED */
    gpiod_set_value(gpio_out, state);

    pr_info("sai_gpio_irq: Interrupt! Input=%d, LED updated\n", state);

    return IRQ_HANDLED;
}

static int __init sai_init(void)
{
    int ret;

    pr_info("sai_gpio_irq: loading...\n");

    gpio_in = gpio_to_desc(GPIO_IN);
    gpio_out = gpio_to_desc(GPIO_OUT);

    if (!gpio_in || !gpio_out)
        return -EINVAL;

    /* Input and output setup */
    ret = gpiod_direction_input(gpio_in);
    if (ret) return ret;

    ret = gpiod_direction_output(gpio_out, 0);
    if (ret) return ret;

    /* Convert GPIO → IRQ */
    irq_line = gpiod_to_irq(gpio_in);
    if (irq_line < 0) {
        pr_err("Failed to get IRQ\n");
        return irq_line;
    }

    /* Request threaded IRQ */
    ret = request_threaded_irq(
        irq_line,
        NULL,                   // top half (unused)
        sai_irq_handler,        // threaded handler
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
        DEVICE_NAME,
        NULL);

    if (ret) {
        pr_err("Failed to request IRQ\n");
        return ret;
    }

    pr_info("sai_gpio_irq: IRQ requested on line %d\n", irq_line);
    return 0;
}

static void __exit sai_exit(void)
{
    free_irq(irq_line, NULL);
     /* turn off LED */
        gpiod_set_value(gpio_out, 0);
        pr_info("sai_gpio_irq: Led off\n");
    pr_info("sai_gpio_irq: unloaded\n");
}

module_init(sai_init);
module_exit(sai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("GPIO IRQ driver example");
