#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void)
{
    printk(KERN_INFO "Hello world! Linux driver is loaded.\n");
    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "Goodbye Sai! Driver unloaded.\n");
}

MODULE_LICENSE("GPL");
