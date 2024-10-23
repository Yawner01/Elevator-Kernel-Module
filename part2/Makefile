obj-m += my_timer.o
ccflags-y := ig -Wno-error
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
SRC := src
BIN := bin
KMOD := my_timer

all: $(BIN) $(BIN)/$(KMOD).ko

$(BIN):
	mkdir -p $(BIN)

$(BIN)/$(KMOD).ko: src/my_timer.c | $(BIN)
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules
	cp src/$(KMOD).ko $(BIN)/

load: $(BIN)/$(KMOD).ko
	sudo insmod $(BIN)/$(KMOD).ko

unload:
	sudo rmmod $(KMOD)

test: all
	@echo "Starting test for my_timer kernel module..."

	@echo "Reading /proc/timer"
	@cat /proc/timer

	@echo "Sleeping for 1 second..."
	@sleep 1

	@echo "Reading /proc/timer after 1 second"
	@cat /proc/timer

	@echo "sleeping for 3 seconds..."
	@sleep 3

	@echo "Reading /proc/timer after 3 seconds"
	@cat /proc/timer

	@echo "Sleeping for 5 seconds..."
	@sleep 5

	@echo "Reading /proc/timer after 5 seconds"
	@cat /proc/timer

	@echo "Test completed"

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
	rm -f $(BIN)/$(KMOD).ko

.PHONY: all load unload test clean
