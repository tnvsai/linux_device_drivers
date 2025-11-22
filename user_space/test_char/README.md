User-Space Test Program for sai_char Driver

This C program communicates with the /dev/sai_char character device created by the sai_char kernel driver.
It demonstrates how user applications can perform:

open()

write()

read()

close()

to interact with a Linux character device.

1. Usage
Compile

Run inside the user_space/test_char/ directory:

make


This builds the executable:

test_sai_char

Execute
Reading from the driver
./test_sai_char read

Writing to the driver
./test_sai_char write "hello kernel"

2. Program Flow
2.1 open()

The program opens the device using:

open("/dev/sai_char", O_RDWR);


This triggers the driver's open() handler.

2.2 write()

If the command is:

./test_sai_char write "hello"


Then:

The message is written to the driver using write()

The driver's sai_write() function receives the data

The data is stored inside the driver's internal kernel_buffer

2.3 read()

If the command is:

./test_sai_char read


Then:

The program issues a read() system call

The driver's sai_read() handler is executed

The driver copies data from its kernel_buffer[] into the user-space buffer

Example output:

Data from driver: hello kernel

3. Error Handling

The program includes checks for:

Failure to open the device

Missing arguments for write

Read or write errors

4. Summary

This user-space program demonstrates:

How applications access kernel drivers through /dev nodes

How system calls (read, write) reach driver callbacks

How data flows between user space and kernel space
