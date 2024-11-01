#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

// module info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 - Group 2");
MODULE_DESCRIPTION("Elevator kernel module");
MODULE_VERSION("1.0");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

// elevator states
#define IDLE 0
#define OFFLINE 1
#define MOVING_UP 2
#define MOVING_DOWN 3
#define LOADING_UNLOADING 4
#define DEACTIVATING 5

// elevator restrictions
#define MIN_FLOOR 1
#define MAX_FLOOR 6
#define MAX_PASSENGERS 5
#define MAX_WEIGHT 750

// passenger types
#define FRESHMAN 0
#define SOPHOMORE 1
#define JUNIOR 2
#define SENIOR 3

// passenger weights
#define FRESHMAN_WEIGHT 100
#define SOPHOMORE_WEIGHT 150
#define JUNIOR_WEIGHT 200
#define SENIOR_WEIGHT 250

// passenger definition
struct passenger {
	int start_floor;
	int destination_floor;
	int type;
	bool on_elevator;
	struct list_head list;
};

// elevator definition
struct elevator {
	int state;
	bool deactivating;
	int current_floor;
	int direction;
	int num_passengers;
	int total_weight;
	struct list_head passengers;
	struct mutex lock;
	struct task_struct* thread;
} dorm_elevator;

static int total_passengers_waiting = 0;
static int total_passengers_serviced = 0;

extern int(*STUB_start_elevator)(void);
extern int(*STUB_issue_request)(int, int, int);
extern int(*STUB_stop_elevator)(void);

static int elevator_proc_show(struct seq_file *m, void *v);
static int elevator_proc_open(struct inode *inode, struct file *file);

static const struct proc_ops elevator_proc_fops = {
	.proc_open = elevator_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static char get_passenger_type(int type) {
	switch(type) {
		case FRESHMAN: return 'F';
		case SOPHOMORE: return 'O';
		case JUNIOR: return 'J';
		case SENIOR: return 'S';
		default: return '?'; //for invalid type
		}
}

static int elevator_thread(void* data) {
	while (!kthread_should_stop()) {
		mutex_lock(&dorm_elevator.lock);

		if (dorm_elevator.state == OFFLINE) {
			mutex_unlock(&dorm_elevator.lock);
			ssleep(1);
			continue;
		}

		if (list_empty(&dorm_elevator.passengers)) {
			printk(KERN_INFO "Inside IDLE\n");
			if(dorm_elevator.deactivating) {
				dorm_elevator.state = OFFLINE;
				dorm_elevator.deactivating = false;
				printk(KERN_INFO "Deactivating is TRUE\n");
			}
			else {
				printk(KERN_INFO "Deactivating is FALSE\n");
				dorm_elevator.state = IDLE;
			}
			mutex_unlock(&dorm_elevator.lock);
			ssleep(1);
			continue;
		}

		if (dorm_elevator.deactivating && list_empty(&dorm_elevator.passengers)) {
                        dorm_elevator.state = OFFLINE;
                        dorm_elevator.deactivating = false;
                        mutex_unlock(&dorm_elevator.lock);
                        printk(KERN_INFO "Elevator is now OFFLINE\n");
                        break;
                }

		if (dorm_elevator.state == IDLE) {
			struct passenger* first_p = list_first_entry(&dorm_elevator.passengers, struct passenger, list);
			if (first_p->destination_floor > dorm_elevator.current_floor)
				dorm_elevator.direction = 1;
			else
				dorm_elevator.direction = -1;
			dorm_elevator.state = (dorm_elevator.direction == 1) ? MOVING_UP : MOVING_DOWN;
		}

		if (dorm_elevator.state == MOVING_UP || dorm_elevator.state == MOVING_DOWN) {
			dorm_elevator.current_floor += dorm_elevator.direction;
			printk(KERN_INFO "Elevator moved to floor %d\n", dorm_elevator.current_floor);

			if (dorm_elevator.current_floor > MAX_FLOOR) {
				dorm_elevator.current_floor = MAX_FLOOR;
				dorm_elevator.direction = -1;
				dorm_elevator.state = MOVING_DOWN;
			} else if (dorm_elevator.current_floor < MIN_FLOOR) {
				dorm_elevator.current_floor = MIN_FLOOR;
				dorm_elevator.direction = 1;
				dorm_elevator.state = MOVING_UP;
			}

			mutex_unlock(&dorm_elevator.lock);
			ssleep(2);
			mutex_lock(&dorm_elevator.lock);

			struct passenger *p, *tmp;
			list_for_each_entry_safe(p, tmp, &dorm_elevator.passengers, list) {
				if (p->destination_floor == dorm_elevator.current_floor && p->on_elevator) {
					dorm_elevator.num_passengers--;
					dorm_elevator.total_weight -= (p->type == FRESHMAN) ? FRESHMAN_WEIGHT :
									(p->type == SOPHOMORE) ? SOPHOMORE_WEIGHT :
									(p->type == JUNIOR) ? JUNIOR_WEIGHT :
									SENIOR_WEIGHT;
					list_del(&p->list);
					kfree(p);
					total_passengers_serviced++;
					dorm_elevator.state = LOADING_UNLOADING;
					printk(KERN_INFO "Passenger serviced at floor %d\n", dorm_elevator.current_floor);
				}
			}
			list_for_each_entry_safe(p, tmp, &dorm_elevator.passengers, list) {
				if (p->start_floor == dorm_elevator.current_floor && !p->on_elevator) {
					int passenger_weight = (p->type == FRESHMAN) ? FRESHMAN_WEIGHT :
								(p->type == SOPHOMORE) ? SOPHOMORE_WEIGHT :
								(p->type == JUNIOR) ? JUNIOR_WEIGHT :
								SENIOR_WEIGHT;
					if ((dorm_elevator.num_passengers < MAX_PASSENGERS) &&
					   ((dorm_elevator.total_weight + passenger_weight) <= MAX_WEIGHT)) {
						dorm_elevator.num_passengers++;
						dorm_elevator.total_weight += passenger_weight;
						p->on_elevator = true;
						total_passengers_waiting--;
						printk(KERN_INFO "Passenger loaded at floor %d\n", dorm_elevator.current_floor);
						dorm_elevator.state = LOADING_UNLOADING;
					} else {
						// Full
						break;
					}
				}
			}
			if(dorm_elevator.state == LOADING_UNLOADING) {
				mutex_unlock(&dorm_elevator.lock);
				ssleep(1);
				mutex_lock(&dorm_elevator.lock);
			}
			dorm_elevator.state = (dorm_elevator.direction == 1) ? MOVING_UP : MOVING_DOWN;
			mutex_unlock(&dorm_elevator.lock);
		} else {
			// unknown
			mutex_unlock(&dorm_elevator.lock);
			ssleep(1);
		}
	}
	return 0;
}

//function to display the elevator state in the proc file
static int elevator_proc_show(struct seq_file *m, void *v) {
	mutex_lock(&dorm_elevator.lock);

	char *state_str;
	switch(dorm_elevator.state) {
		case OFFLINE: state_str = "OFFLINE"; break;
		case IDLE: state_str = "IDLE"; break;
		case MOVING_UP: state_str = "MOVING_UP"; break;
		case MOVING_DOWN: state_str = "MOVING_DOWN"; break;
		case LOADING_UNLOADING: state_str = "LOADING/UNLOADING"; break;
		default: state_str = "UNKNOWN"; break;
	}

	seq_printf(m, "Elevator state: %s\n", state_str);
	seq_printf(m, "Current floor: %d\n", dorm_elevator.current_floor);
	seq_printf(m, "Current load: %d lbs\n", dorm_elevator.total_weight);
	seq_printf(m, "Elevator status: ");

	struct passenger *p;
	list_for_each_entry(p, &dorm_elevator.passengers, list) {
		char passenger_type = get_passenger_type(p->type);
		if (p->on_elevator) {
			seq_printf(m, "%c%d ", passenger_type, p->destination_floor);
		}
	}
	seq_printf(m, "\n\n");

	//floor by floor information
	for (int floor = MAX_FLOOR; floor >= MIN_FLOOR; floor--) {
		if (dorm_elevator.current_floor == floor) {
			seq_printf(m, "[*] Floor %d: ", floor); //floor evelavtor is on
			} else {
			seq_printf(m, "[] Floor %d: ", floor); //floor elevlator is not on
			}

		int passengers_waiting_on_floor = 0;
		list_for_each_entry(p, &dorm_elevator.passengers, list) {
			if (p->start_floor == floor && !p->on_elevator) {
				passengers_waiting_on_floor++;
			}
		}
		seq_printf(m, "%d ", passengers_waiting_on_floor);

		list_for_each_entry(p, &dorm_elevator.passengers, list) {
			if (p->start_floor == floor && !p->on_elevator) {
				char passenger_type = get_passenger_type(p->type);
				seq_printf(m, "%c%d ", passenger_type, p->destination_floor);
			}
		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nNumber of passengers: %d\n", dorm_elevator.num_passengers);
	seq_printf(m, "Number of passengers waiting: %d\n", total_passengers_waiting);
	seq_printf(m, "Number of passengers serviced: %d\n", total_passengers_serviced);
	mutex_unlock(&dorm_elevator.lock);
	return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file) {
	return single_open(file, elevator_proc_show, NULL);
}

static int start_elevator(void) {
	printk(KERN_NOTICE "Starting the elevator...\n");
	mutex_lock(&dorm_elevator.lock);
	if (dorm_elevator.state != OFFLINE) {
		mutex_unlock(&dorm_elevator.lock);
		return 1; // elevator already active
	}

	// init elevator
	dorm_elevator.deactivating = false;
	dorm_elevator.state = IDLE;
	dorm_elevator.current_floor = MIN_FLOOR;
	dorm_elevator.direction = 1;
	dorm_elevator.num_passengers = 0;
	dorm_elevator.total_weight = 0;
	INIT_LIST_HEAD(&dorm_elevator.passengers);
	total_passengers_waiting = 0;
	total_passengers_serviced = 0;
	mutex_unlock(&dorm_elevator.lock);

	return 0; // successful start
}

static int issue_request(int start_floor, int destination_floor, int type) {
    struct passenger* new_passenger;

	if (start_floor < MIN_FLOOR || destination_floor < MIN_FLOOR ||
	    start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR ||
	    type < FRESHMAN || type > SENIOR)
		return 1; // invalid request

	mutex_lock(&dorm_elevator.lock);
	if (dorm_elevator.deactivating == true || dorm_elevator.state == OFFLINE) {
		mutex_unlock(&dorm_elevator.lock);
		return 1;
	}
	mutex_unlock(&dorm_elevator.lock);

	new_passenger = kmalloc(sizeof(*new_passenger), GFP_KERNEL);
	if (!new_passenger) {
		return -ENOMEM; // memory allocation failed
	}

	new_passenger->start_floor = start_floor;
	new_passenger->destination_floor = destination_floor;
	new_passenger->type = type;
	new_passenger->on_elevator = false;
	INIT_LIST_HEAD(&new_passenger->list);

	mutex_lock(&dorm_elevator.lock);
	list_add_tail(&new_passenger->list, &dorm_elevator.passengers);
	total_passengers_waiting++; //increment passengers waiting
	mutex_unlock(&dorm_elevator.lock);
	return 0; // successful request
}

static int stop_elevator(void) {
    printk(KERN_NOTICE "Stopping the elevator...\n");
	mutex_lock(&dorm_elevator.lock);
	if (dorm_elevator.deactivating || dorm_elevator.state == OFFLINE) {
		mutex_unlock(&dorm_elevator.lock);
		return 1; // elevator already offline
	}

	dorm_elevator.deactivating = true;
	mutex_unlock(&dorm_elevator.lock);
	return 0;
}

//initialize module and proc file
static int __init elevator_module_init(void) {
	printk(KERN_INFO "Elevator module initializing...\n");

    	STUB_start_elevator=start_elevator;
    	STUB_issue_request=issue_request;
    	STUB_stop_elevator=stop_elevator;

	//create proc entry
	proc_create("elevator", 0, NULL, &elevator_proc_fops);

	//init elevator state
	dorm_elevator.state = OFFLINE;
	dorm_elevator.deactivating = false;
	dorm_elevator.current_floor = MIN_FLOOR;
	dorm_elevator.direction = 1;
	dorm_elevator.num_passengers = 0;
	dorm_elevator.total_weight = 0;
	INIT_LIST_HEAD(&dorm_elevator.passengers);
	mutex_init(&dorm_elevator.lock);

	dorm_elevator.thread = kthread_run(elevator_thread, NULL, "elevator_thread");
	if (IS_ERR(dorm_elevator.thread)) {
		printk(KERN_ERR "Failed to create elevator thread\n");
		remove_proc_entry("elevator", NULL);
		return PTR_ERR(dorm_elevator.thread);
	}

	return 0;
}

//clean module and remove proc file
static void __exit elevator_module_exit(void) {
	printk(KERN_INFO "Elevator module exiting...\n");

    	STUB_start_elevator=NULL;
    	STUB_issue_request=NULL;
    	STUB_stop_elevator=NULL;

	printk(KERN_INFO "Elevator syscalls set to NULL\n");

	if(dorm_elevator.thread) {
		printk(KERN_INFO "Stopping Elevator thread\n");
		kthread_stop(dorm_elevator.thread);
		dorm_elevator.thread = NULL;
	}
	printk(KERN_INFO "Elevator thread stopped\n");

	remove_proc_entry("elevator", NULL);
	printk(KERN_INFO "Proc removed\n");
}

module_init(elevator_module_init);
module_exit(elevator_module_exit);
