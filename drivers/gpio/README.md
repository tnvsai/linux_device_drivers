sai_gpio – Simple GPIO Character Driver for Raspberry Pi

Author: Sai
Version: 1.0
License: GPL

This project implements a simple Linux GPIO driver that exposes a GPIO pin as a character device in /dev.
User-space applications can control the GPIO using standard file operations (read, write, open, close).

The driver supports:

Setting GPIO output HIGH or LOW

Reading current GPIO state

Clean GPIO operations using modern kernel APIs (gpiod)

Compatible with Raspberry Pi OS (Bookworm/Trixie, Kernel 6.x)

1. Overview

Raspberry Pi exposes GPIOs through the kernel as:

gpiochip0: GPIOs 512–569


Therefore:

BCM GPIO N → Kernel GPIO = 512 + N


Example:
BCM 17 → kernel GPIO 529

The driver controls BCM 17 using /dev/sai_gpio.

2. GPIO Debug & Inspection Commands (Important Section)

These commands help inspect GPIO availability, pin mapping, conflicts, and kernel configuration.

2.1 View all GPIOs managed by the kernel
sudo cat /sys/kernel/debug/gpio


Example output (your Pi):

gpiochip0: GPIOs 512-569, pinctrl-bcm2711
  gpio-529 (GPIO17)
  gpio-533 (GPIO21)
  ...


Shows:

Kernel GPIO number

Pin name (GPIO17, GPIO21, etc.)

Whether the pin is already in use

The controller (pinctrl-bcm2711)

2.2 Check gpiochips and GPIO capabilities

Install tools (once):

sudo apt install gpiod


Then:

Detect gpiochips:
gpiodetect

List all pins serviced by each chip:
gpioinfo

2.3 Check if a GPIO is free using legacy sysfs
sudo sh -c 'echo 17 > /sys/class/gpio/export'


If busy → you will see:

Device or resource busy


If successful → sysfs folder appears:

/sys/class/gpio/gpio17/


(Your kernel may disable sysfs, but command is useful to test pin availability.)

2.4 Check who is using a GPIO
sudo cat /sys/kernel/debug/gpio | grep "529"


Replace 529 with your kernel GPIO number.

2.5 Confirm your device node exists
ls -l /dev/sai_gpio

2.6 Kernel logs related to GPIO
dmesg | grep sai_gpio


Shows errors such as:

gpio_to_desc failed

cannot request GPIO

Pin conflicts

3. Driver Features

/dev/sai_gpio device node

Write "1" or "0" to control GPIO output

Read pin state ("1" or "0")

Clean gpiod-based design

Uses modern GPIO APIs

4. Kernel APIs Used
gpio_to_desc()

Converts kernel GPIO number → descriptor.

gpiod_direction_output()

Configures pin as output.

gpiod_set_value()

Sets HIGH/LOW.

gpiod_get_value()

Reads logic level.

Character device APIs:

alloc_chrdev_region()

cdev_init()

cdev_add()

class_create()

device_create()

5. Build Instructions
make


Produces:

sai_gpio.ko


Clean:

make clean

6. Loading the Driver
sudo insmod sai_gpio.ko


Logs:

dmesg | tail


Device node:

ls -l /dev/sai_gpio

7. Usage
GPIO HIGH
echo 1 | sudo tee /dev/sai_gpio

GPIO LOW
echo 0 | sudo tee /dev/sai_gpio

Read GPIO
cat /dev/sai_gpio

8. Unloading
sudo rmmod sai_gpio

9. Code Summary

This driver teaches:

Mapping BCM → kernel GPIO numbering

Basic GPIO control from kernel

Creating a character device interface

Handling read/write

Using recommended gpiod APIs

Simple, clean beginner-friendly driver design

10. Notes

To use a different GPIO, set:

GPIO_NUM = 512 + BCM


Always check with:

sudo cat /sys/kernel/debug/gpio


to confirm the pin is free.