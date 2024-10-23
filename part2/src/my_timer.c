#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610-group2");
MODULE_DESCRIPTION("A kernel module that tracks current time and elapsed time.");
MODULE_VERSION("1.0");

#define PROC_NAME "timer"
#define PARENT NULL
#define PERMS 0444

struct timespec64 current_ts;
struct timespec64 previous_ts;

#define BUF_LEN 256
static struct proc_dir_entry* proc_entry;
static char buffer[BUF_LEN];
static ssize_t buffer_size = 0;
static DEFINE_MUTEX(timer_mutex);

static ssize_t timer_proc_read(struct file* file, char __user *user_buffer, size_t count, loff_t *ppos) {
	ssize_t ret;
	if(*ppos > 0 || count < BUF_LEN) return 0;
	if(!mutex_trylock(&timer_mutex)) {
		printk(KERN_ALERT "my_timer: Failed to acquire mutex\n");
		return -EBUSY;
	}

	ktime_get_real_ts64(&current_ts);

	if(previous_ts.tv_sec != 0 || previous_ts.tv_nsec != 0) {
		struct timespec64 elapsed;
		elapsed.tv_sec = current_ts.tv_sec - previous_ts.tv_sec;
		elapsed.tv_nsec = current_ts.tv_nsec - previous_ts.tv_nsec;

		if(elapsed.tv_nsec < 0) {
			elapsed.tv_sec -= 1;
			elapsed.tv_nsec += 1000000000L;
		}

		buffer_size = scnprintf(buffer, BUF_LEN, "current time: %lld.%09ld\nelapsed time: %lld.%09ld\n",
					(long long) current_ts.tv_sec, current_ts.tv_nsec,
					(long long) elapsed.tv_sec, elapsed.tv_nsec);

	} else {
		buffer_size = scnprintf(buffer, BUF_LEN, "current time: %lld.%09ld\n", (long long) current_ts.tv_sec,
					current_ts.tv_nsec);
	}

	previous_ts = current_ts;
	mutex_unlock(&timer_mutex);

	ret = simple_read_from_buffer(user_buffer, count, ppos, buffer, buffer_size);
	return ret;
}

static const struct proc_ops timer_proc_ops = {
	.proc_read = timer_proc_read,
};

static int __init my_timer_init(void) {
	printk(KERN_INFO "my_timer: Initializing the my_timer module \n");

	previous_ts.tv_sec = 0;
	previous_ts.tv_nsec = 0;
	proc_entry = proc_create(PROC_NAME, PERMS, PARENT, &timer_proc_ops);

	if(!proc_entry) {
		printk(KERN_ALERT "my_timer: Failed to create /proc/%s\n", PROC_NAME);
		return -ENOMEM;
	}

	printk(KERN_INFO "my_timer: /proc/%s created\n", PROC_NAME);
	return 0;
}

static void __exit my_timer_exit(void) {
	if(proc_entry) {
		proc_remove(proc_entry);
		printk(KERN_INFO "my_timer: /proc/%s removed\n", PROC_NAME);
	}
	printk(KERN_INFO "my_timer: Goodbye from the my_timer module\n");
}

module_init(my_timer_init);
module_exit(my_timer_exit);

