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

// module info
MODULE_LICENSE("GPL");
MODULE_AUTHOR("COP4610 - Group 2");
MODULE_DESCRIPTION("Elevator kernel module");
MODULE_VERSION("1.0");
//MODULE_NAME("elevator");

// elevator states
#define IDLE 0
#define OFFLINE 1
#define MOVING 2
#define LOADING_UNLOADING 3

// elevator restrictions
#define MIN_FLOOR 1
#define MAX_FLOOR 6

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
	struct list_head list;
};

// elevator definition
struct elevator {
	int state;
	int current_floor;
	int num_passengers;
	int total_weight;
	struct list_head passengers;
	struct mutex lock;
	struct task_struct* thread;
};

// global elevator instance
static struct elevator dorm_elevator = {
	.state = OFFLINE,
	.current_floor = MIN_FLOOR,
	.num_passengers = 0,
	.total_weight = 0,
	.passengers = LIST_HEAD_INIT(dorm_elevator.passengers)
};

// sys call stubs
int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;

EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

static int elevator_thread(void *data) {
	while (!kthread_should_stop()) {
		mutex_lock(&dorm_elevator.lock);
		
		// elevator movement logic
		if (dorm_elevator.state == IDLE && !list_empty(&dorm_elevator.passengers)) {
			struct passenger* p;
			list_for_each_entry(p, &dorm_elevator.passengers, list) {
				if (p->start_floor == dorm_elevator.current_floor) {
					// load passenger
					++dorm_elevator.num_passengers;
					switch (p->type) {
						case 0: dorm_elevator.total_weight += FRESHMAN_WEIGHT; break;
						case 1: dorm_elevator.total_weight += SOPHOMORE_WEIGHT; break;
						case 2: dorm_elevator.total_weight += JUNIOR_WEIGHT; break;
						case 3: dorm_elevator.total_weight += SENIOR_WEIGHT; break;
					}
					list_del(&p->list);
					kfree(p);
					ssleep(1); // loading time
				}
			}
		}
		
		mutex_unlock(&dorm_elevator.lock);
		ssleep(2); // moving time
	}
	
	return 0;
}

// sys call wrappers
SYSCALL_DEFINE0(start_elevator) {
	printk(KERN_NOTICE "Starting the elevator...\n");
	if (dorm_elevator.state != OFFLINE)
		return 1; // elevator already active

	// init elevator
	dorm_elevator.state = IDLE;
	dorm_elevator.current_floor = MIN_FLOOR;
	dorm_elevator.num_passengers = 0;
	dorm_elevator.total_weight = 0;
	INIT_LIST_HEAD(&dorm_elevator.passengers);
	mutex_init(&dorm_elevator.lock);
	dorm_elevator.thread = kthread_run(elevator_thread, NULL, "elevator_thread");
	return 0; // successful start
}

SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type) {
	struct passenger* new_passenger;
	
	if (start_floor < MIN_FLOOR || destination_floor < MIN_FLOOR || start_floor > MAX_FLOOR || destination_floor > MAX_FLOOR || type < FRESHMAN || type > SENIOR)
		return 1; // invalid request

	new_passenger = kmalloc(sizeof(*new_passenger), GFP_KERNEL);
	if (!new_passenger) {
		return -ENOMEM; // memory allocation failed
	}
	
	new_passenger->start_floor = start_floor;
	new_passenger->destination_floor = destination_floor;
	new_passenger->type = type;
	INIT_LIST_HEAD(&new_passenger->list);
	
	mutex_lock(&dorm_elevator.lock);
	list_add_tail(&new_passenger->list, &dorm_elevator.passengers);
	mutex_unlock(&dorm_elevator.lock);
	
	return 0; // successful request
}

SYSCALL_DEFINE0(stop_elevator) {
	printk(KERN_NOTICE "Stopping the elevator...\n");
	if (dorm_elevator.state == OFFLINE)
		return 1; // elevator already offline

	// make sure all passengers are offloaded
	mutex_lock(&dorm_elevator.lock);
	if (dorm_elevator.num_passengers == 0) {
		dorm_elevator.state = OFFLINE;
		if (dorm_elevator.thread)
			kthread_stop(dorm_elevator.thread);
		mutex_unlock(&dorm_elevator.lock);
		return 0; // successful stop
	} else {
		printk(KERN_NOTICE "Elevator still has passengers\n");
		mutex_unlock(&dorm_elevator.lock);
		return 1; // cannot stop yet
	}
}
