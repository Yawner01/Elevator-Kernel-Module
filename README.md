# Project 2

This project aims to provide a comprehensive understanding of system calls, kernel programming, concurrency, synchronization, and elevator scheduling algorithms. It consists of multiple parts that build upon each other to deepen your knowledge and skills in these areas.

## Group Members
- **Wesley Yawn**: way23@fsu.edu
- **Jose Rodriguez**: jmr21a@fsu.edu
- **Eli Butters**: ebb21g@fsu.edu
## Division of Labor

### Part 1: System-Call Tracing
- **Responsibilities**: Create a program that has exactly 4 system calls.
- **Assigned to**: Eli Butters

### Part 2: Timer Kernel Module
- **Responsibilities**: In Unix-like operating systems, the time is often represented as the number of seconds since the Unix Epoch (January 1st, 1970). The task requires creating a kernel module named "my_timer" that utilizes the function ktime_get_real_ts64() to retrieve the time value, which includes seconds and nanoseconds since the Epoch.
- **Assigned to**: Eli Butters

### Part 3: Elevator Module
- #### Step 1: Add system Calls
    - **Responsibilities**: Download the latest linux kernel and add three system calls to control the elevator.
    - **Assigned to**: Jose Rodriguez, Eli Butters
- #### Step 2: Compile and Install the Kernel
    - **Responsibilities**: Compile the kernel with the new system calls.
    - **Assigned to**: Jose Rodriguez
- #### Step 3: Test System Calls
    - **Responsibilities**: Test if the added system calls work.
    - **Assigned to**: Jose Rodriguez
- #### Step 4: Implement Elevator
    - **Responsibilities**: Implement the elevator kernel module.
    - **Assigned to**: Jose Rodriguez, Eli Butters
- #### Step 5: Write to Proc File
    - **Responsibilities**: Provide an entry in /proc/elevator to display the elevators actions.
    - **Assigned to**: Wesley Yawn
- #### Step 6: Test Elevator
    - **Responsibilities**: Interact with the provided user-space applications to communicate with the kernel module
    - **Assigned to**: Eli Butters

## File Listing
```
root/
└── part1/
    └── bin/
        └── .gitkeep
    └── Makefile
    └── empty.c
    └── empty.trace
    └── part1.c
    └── part1.trace
└── part2/
    └── bin/
        └── .gitkeep
    └── src/
        └── Makefile
        └── my_timer.c
    └── Makefile
└── part3/
    └── bin/
        └── .gitkeep
    └── src/
        └── Makefile
        └── elevator.c
    └── Makefile
    └── syscalls.c
└── .gitignore
└── README.md

```
## How to Compile & Execute

### Requirements
- **Compiler**: `gcc`
- **Dependencies**:

### Part 1:

- ### Compilation
    ```bash
    make -C part1 all
    ```
    This will build the executable in /part1/bin/

- ### Execution
    ```bash
    make -C part1 empty
    make -C part1 part1
    ```
    This will run the program in /part1/bin/

### Part 2:

- ### Compilation
    ```bash
    make -C part2 all
    ```
    This will build the kernel module in /part2/bin/

- ### Loading
    ```bash
    make -C part2 load
    ```
    This will load the kernel module in /part2/bin/

- ### Testing
    ```bash
    make -C part2 test
    ```
    This will test the kernel module in /part2/bin/

- ### Unloading
    ```bash
    make -C part2 unload
    ```
    This will unload the kernel module in /part2/bin/

### Part 3:
- ### Compilation
    ```bash
    make -C part3 all
    ```
    This will compile the elevator kernel module in /part2/bin/

- ### Loading
    ```bash
    make -C part3 load
    ```
    This will load the elevator kernel module in /part3/bin/

- ### Unloading
    ```bash
    make -C part3 unload
    ```
    This will unload the elevator kernel module in /part3/bin/

## Bugs
- **Bug 1**: No current bugs

## Considerations
This project is designed for a linux-kernel-6.10.x running on our assigned virtual machine. Running this on any other kernel design could lead to errors.