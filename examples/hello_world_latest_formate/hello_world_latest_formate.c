#include <linux/module.h>
#include <linux/kernel.h>

static int __init my_init(void)
{
    printk(KERN_INFO "Latest formate Module loaded. Hello Sai!\n");
    return 0;
}

static void __exit my_exit(void)
{
    printk(KERN_INFO "Latest formate Module unloaded. Goodbye Sai!\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NAGA VENKATA SAI");
MODULE_DESCRIPTION("Lesson 2: Demonstration of modern kernel module structure");
MODULE_VERSION("1.0");
