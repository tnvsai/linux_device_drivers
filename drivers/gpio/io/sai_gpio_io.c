// sai_gpio_io.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>

#define DEVICE_NAME "sai_gpio_io"

#define GPIO_IN   539   // BCM 27 (Pin 13)
#define GPIO_OUT  529   // BCM 17 (Pin 11)


static dev_t dev_num;
static struct cdev sai_cdev;
static struct class *sai_class;
static struct device *sai_device;

static struct gpio_desc *gpio_in;
static struct gpio_desc *gpio_out;

/* open */
static int sai_open(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio_io: opened\n");
    return 0;
}

/* close */
static int sai_close(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio_io: closed\n");
    return 0;
}

/* read:
 * - read input pin (BCM13)
 * - set output pin (BCM11) to same value
 * - return '1' or '0'
*/
static ssize_t sai_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
    char out;
    int val;

    if (*ppos > 0)
        return 0; //EOF

    /* read input pin */
    val = gpiod_get_value(gpio_in);

    /* mirror to output pin */
    gpiod_set_value(gpio_out, val);

    out = val ? '1' : '0';

    if (copy_to_user(buf, &out, 1))
        return -EFAULT;

    *ppos = 1;
    return 1;
}

/* write disabled (optional) */
static ssize_t sai_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    return -EINVAL; //not used
}

static const struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = sai_open,
    .release = sai_close,
    .read    = sai_read,
    .write   = sai_write,
};

static int __init sai_init(void)
{
    int ret;

    pr_info("sai_gpio_io: loading...\n");

    /* map descriptors */
    gpio_in = gpio_to_desc(GPIO_IN);
    gpio_out = gpio_to_desc(GPIO_OUT);

    if (!gpio_in || !gpio_out) {
        pr_err("Failed to map GPIOs\n");
        return -EINVAL;
    }

    /* input pin */
    ret = gpiod_direction_input(gpio_in);
    if (ret) {
        pr_err("Failed input direction\n");
        return ret;
    }

    /* output pin default low */
    ret = gpiod_direction_output(gpio_out, 0);
    if (ret) {
        pr_err("Failed output direction\n");
        return ret;
    }

    /* register char device */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret) return ret;

    cdev_init(&sai_cdev, &fops);
    ret = cdev_add(&sai_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    sai_class = class_create(DEVICE_NAME);
    if (IS_ERR(sai_class)) {
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sai_class);
    }

    sai_device = device_create(sai_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sai_device)) {
        class_destroy(sai_class);
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sai_device);
    }

    pr_info("Created /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit sai_exit(void)
{
    device_destroy(sai_class, dev_num);
    class_destroy(sai_class);
    cdev_del(&sai_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("sai_gpio_io: unloaded\n");
}

module_init(sai_init);
module_exit(sai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("GPIO INPUT→OUTPUT simple driver");
