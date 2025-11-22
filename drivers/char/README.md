sai_char – Simple Linux Character Device Driver

Author: NAGA VENKATA SAI
Version: 1.0
License: GPL

This project implements a simple Linux character device driver for learning and demonstration purposes.
It provides the basic file operations:

open()

read()

write()

release()

and creates a device node:

/dev/sai_char

1. Driver Purpose

The driver exposes a character device that allows communication between user-space applications and the Linux kernel using standard file operations.
A small in-kernel buffer (kernel_buffer) is used to store data written from user space, which is returned during read operations.

2. Important Kernel APIs Used
2.1 alloc_chrdev_region()

Dynamically allocates:

Major number

Minor number

Device name (visible in /proc/devices)

This assigns an identity to the character device.

2.2 cdev_init() / cdev_add()

Initializes and registers the cdev structure.
This connects:

major, minor → driver → file_operations


Meaning the kernel knows which driver handles a particular device node.

2.3 class_create()

Creates a device class under:

/sys/class/sai_char/


This helps the kernel organize devices under a common class.

2.4 device_create()

Automatically creates the device node:

/dev/sai_char


This allows user-space programs to access the driver.

2.5 mutex_lock() / mutex_unlock()

Used to protect shared data (kernel_buffer) and ensure thread-safe read/write operations.

3. File Operations Implemented
open()

Logs when the device is opened.

write()

Copies data from user space to kernel space using copy_from_user()

Stores data into kernel_buffer

Resets file offset (*ppos = 0)

read()

Returns previously written data to user space

Handles partial reads (important for tools like cat)

Returns 0 when all data is consumed (EOF)

release()

Logs when the device is closed.

4. Kernel Buffer
static char kernel_buffer[256];
static size_t data_size;
static DEFINE_MUTEX(sai_mutex);


Stores up to 256 bytes

Access is protected by a mutex for thread safety

5. Building the Driver

Run inside the driver directory:

make


This generates:

sai_char.ko

6. Loading the Driver
sudo insmod sai_char.ko


Check kernel logs:

dmesg | tail


Verify device node:

ls -l /dev/sai_char

7. Unloading the Driver
sudo rmmod sai_char

8. Testing

Use the user-space test program located under:

user_space/test_char/


See the dedicated README in that directory for usage instructions.

9. Summary

This driver demonstrates essential Linux driver concepts:

Character device model

Device nodes (/dev)

Major and minor numbers

cdev interface

file_operations callbacks

sysfs class and device creation

Safe kernel/user data exchange with mutex protection