 Kernel Thread (kthread) – Short Description

A kernel thread (kthread) is a lightweight execution context running entirely inside the Linux kernel. Unlike user-space threads, a kthread is scheduled by the kernel and can access kernel APIs, hardware, and internal resources directly.

Kthreads are commonly used in device drivers when you need:

Continuous polling of hardware

Background tasks without blocking user-space

Periodic work (timers, sensor sampling, GPIO monitoring)

Non-interrupt-based logic

A kthread is typically created using:

kthread_run(thread_fn, data, "thread_name");


Inside the thread, the driver usually runs a loop that regularly checks kthread_should_stop() to exit cleanly:

while (!kthread_should_stop()) {
    /* driver logic */
    msleep(5);     // prevent CPU hogging
}


The thread is stopped during module removal using:

kthread_stop(task_struct_pointer);
