// sai_gpio.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>

#define DEVICE_NAME "sai_gpio"

/*
 * NOTE: Use kernel GPIO number (on Raspberry Pi kernels you saw gpiochip0: 512-569)
 * Example: BCM 17 -> kernel GPIO = 512 + 17 = 529
 */
#ifndef GPIO_NUM
#define GPIO_NUM 529
#endif

static dev_t dev_num;
static struct cdev sai_cdev;
static struct class *sai_class;
static struct device *sai_device;
static struct gpio_desc *gpio_pin;

/* FILE OPS */
static int sai_open(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio: opened\n");
    return 0;
}

static int sai_close(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio: closed\n");
    return 0;
}

/* write: single byte '1' or '0' */
static ssize_t sai_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    char ch;

    if (count < 1)
        return -EINVAL;

    if (copy_from_user(&ch, buf, 1))
        return -EFAULT;

    if (ch == '1')
        gpiod_set_value(gpio_pin, 1);
    else
        gpiod_set_value(gpio_pin, 0);

    return count;
}

/* read: returns single byte '1' or '0' */
static ssize_t sai_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
    char out;

    if (*ppos > 0)
        return 0; /* EOF behavior */

    out = gpiod_get_value(gpio_pin) ? '1' : '0';

    if (copy_to_user(buf, &out, 1))
        return -EFAULT;

    *ppos = 1;
    return 1;
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

    pr_info("sai_gpio: loading (GPIO_NUM=%d)\n", GPIO_NUM);

    /* map GPIO number -> descriptor */
    gpio_pin = gpio_to_desc(GPIO_NUM);
    if (!gpio_pin) {
        pr_err("sai_gpio: gpio_to_desc failed for GPIO %d\n", GPIO_NUM);
        return -EINVAL;
    }

    /* configure as output, default low */
    ret = gpiod_direction_output(gpio_pin, 0);
    if (ret) {
        pr_err("sai_gpio: gpiod_direction_output failed (%d)\n", ret);
        return ret;
    }

    /* register character device */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret) {
        pr_err("sai_gpio: alloc_chrdev_region failed (%d)\n", ret);
        return ret;
    }

    cdev_init(&sai_cdev, &fops);
    ret = cdev_add(&sai_cdev, dev_num, 1);
    if (ret) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("sai_gpio: cdev_add failed (%d)\n", ret);
        return ret;
    }

    sai_class = class_create(DEVICE_NAME);
    if (IS_ERR(sai_class)) {
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("sai_gpio: class_create failed\n");
        return PTR_ERR(sai_class);
    }

    sai_device = device_create(sai_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sai_device)) {
        class_destroy(sai_class);
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        pr_err("sai_gpio: device_create failed\n");
        return PTR_ERR(sai_device);
    }

    pr_info("sai_gpio: device created /dev/%s (major=%d minor=%d)\n",
            DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
    return 0;
}


static void __exit sai_exit(void)
{
    device_destroy(sai_class, dev_num);
    class_destroy(sai_class);
    cdev_del(&sai_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("sai_gpio: unloaded\n");
}

module_init(sai_init);
module_exit(sai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("Simple sai_gpio driver (gpiod + gpio_to_desc)");
