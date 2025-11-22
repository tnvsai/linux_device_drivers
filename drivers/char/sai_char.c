#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "sai_char"
#define BUF_SIZE    256

static char kernel_buffer[BUF_SIZE];
static size_t data_size = 0;          // how many bytes are stored
static dev_t dev_num;
static struct cdev sai_cdev;

static struct class *sai_class;
static struct device *sai_device;


/* OPEN */
static int sai_open(struct inode *inode, struct file *file)
{
    pr_info("sai_char: device opened\n");
    return 0;
}

/* CLOSE */
static int sai_close(struct inode *inode, struct file *file)
{
    pr_info("sai_char: device closed\n");
    return 0;
}

/* WRITE */
static ssize_t sai_write(struct file *file, const char __user *user_buf,
                         size_t count, loff_t *ppos)
{
    pr_info("sai_char: write requested (count=%zu)\n", count);

    if (count > BUF_SIZE)
        count = BUF_SIZE;

    if (copy_from_user(kernel_buffer, user_buf, count))
        return -EFAULT;

    data_size = count;      // store data length
    *ppos = 0;              // reset offset after write

    pr_info("sai_char: stored %zu bytes\n", data_size);
    return count;
}

/* READ */
static ssize_t sai_read(struct file *file, char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    size_t bytes_available;
    size_t to_copy;

    pr_info("sai_char: read requested (ppos=%lld, count=%zu)\n",
            (long long)*ppos, count);

    /* No more data left to read (EOF) */
    if (*ppos >= data_size)
        return 0;

    bytes_available = data_size - *ppos;
    to_copy = (count < bytes_available) ? count : bytes_available;

    if (copy_to_user(user_buf, kernel_buffer + *ppos, to_copy))
        return -EFAULT;

    *ppos += to_copy;   // move file pointer

    pr_info("sai_char: sent %zu bytes, new ppos=%lld\n",
            to_copy, (long long)*ppos);

    return to_copy;
}

/* FILE OPERATIONS */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = sai_open,
    .release = sai_close,
    .read    = sai_read,
    .write   = sai_write,
};

/* INIT */
static int __init sai_init(void)
{
    int ret;

    /* Allocate major/minor number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    pr_info("sai_char: registered with major %d minor %d\n",
            MAJOR(dev_num), MINOR(dev_num));

    /* Initialize cdev */
    cdev_init(&sai_cdev, &fops);

    ret = cdev_add(&sai_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    /* Create device class (NEW API: only one argument) */
    sai_class = class_create(DEVICE_NAME);
    if (IS_ERR(sai_class)) {
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sai_class);
    }

    /* Create device node automatically in /dev */
    sai_device = device_create(sai_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sai_device)) {
        class_destroy(sai_class);
        cdev_del(&sai_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(sai_device);
    }

    return 0;
}


/* EXIT */
static void __exit sai_exit(void)
{
    device_destroy(sai_class, dev_num);
    class_destroy(sai_class);

    cdev_del(&sai_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("sai_char: unregistered\n");
}


module_init(sai_init);
module_exit(sai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAGA VENKATA SAI");
MODULE_DESCRIPTION("Corrected Simple Character Driver");
MODULE_VERSION("1.0");
