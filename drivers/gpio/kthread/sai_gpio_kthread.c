// sai_gpio_kthread.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define DEVICE_NAME "sai_gpio_kthread"

/* BCM 27 = Pin 13 → kernel GPIO 539 */
/* BCM 17 = Pin 11 → kernel GPIO 529 */
#define GPIO_IN   539
#define GPIO_OUT  529

static dev_t dev_num;
static struct cdev sai_cdev;
static struct class *sai_class;
static struct device *sai_device;

static struct gpio_desc *gpio_in;
static struct gpio_desc *gpio_out;

/* kernel thread */
static struct task_struct *monitor_thread;

/* Thread function */
static int gpio_monitor_fn(void *data)
{
    int val;

    pr_info("sai_gpio_kthread: monitor thread started\n");

    while (!kthread_should_stop()) {

        /* read input */
        val = gpiod_get_value(gpio_in);

        /* reflect to LED */
        gpiod_set_value(gpio_out, val);

        /* reduce CPU load (5ms) */
        msleep(5);
    }

    pr_info("sai_gpio_kthread: monitor thread stopping\n");
    return 0;
}

/* read: just returns current input state */
static ssize_t sai_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
    char out;
    int val;

    if (*ppos > 0)
        return 0;

    val = gpiod_get_value(gpio_in);
    out = val ? '1' : '0';

    if (copy_to_user(buf, &out, 1))
        return -EFAULT;

    *ppos = 1;
    return 1;
}

static int sai_open(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio_kthread: opened\n");
    return 0;
}

static int sai_close(struct inode *inode, struct file *file)
{
    pr_info("sai_gpio_kthread: closed\n");
    return 0;
}

static ssize_t sai_write(struct file *file,
                         const char __user *buf,
                         size_t count, loff_t *ppos)
{
    return -EINVAL; // not needed
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

    pr_info("sai_gpio_kthread: loading...\n");

    gpio_in = gpio_to_desc(GPIO_IN);
    gpio_out = gpio_to_desc(GPIO_OUT);

    if (!gpio_in || !gpio_out) {
        pr_err("Failed to map GPIOs\n");
        return -EINVAL;
    }

    /* configure direction */
    ret = gpiod_direction_input(gpio_in);
    if (ret) return ret;

    ret = gpiod_direction_output(gpio_out, 0);
    if (ret) return ret;

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

    /* start kernel thread */
    monitor_thread = kthread_run(gpio_monitor_fn, NULL, "gpio_mon_thread");
    if (IS_ERR(monitor_thread)) {
        pr_err("Failed to start kthread\n");
        device_destroy(sai_class, dev_num);
        class_destroy(sai_class);
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(monitor_thread);
    }

    pr_info("sai_gpio_kthread: loaded successfully\n");
    return 0;
}

static void __exit sai_exit(void)
{
    /* stop thread */
    if (monitor_thread)
        kthread_stop(monitor_thread);

        
      /* turn off LED */
        gpiod_set_value(gpio_out, 0);

    device_destroy(sai_class, dev_num);
    class_destroy(sai_class);
    cdev_del(&sai_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("sai_gpio_kthread: unloaded\n");
}

module_init(sai_init);
module_exit(sai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sai");
MODULE_DESCRIPTION("GPIO input-to-output driver with kernel thread");
