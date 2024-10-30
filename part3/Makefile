obj-m += elevator.o
ccflags := ig -Wno-error
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
SRC := src
BIN := bin
KMOD := elevator

all: $(BIN) $(BIN)/$(KMOD).ko

$(BIN):
	mkdir -p $(BIN)

$(BIN)/$(KMOD).ko: src/elevator.c | $(BIN)
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules
	cp src/$(KMOD).ko $(BIN)/

load: $(BIN)/$(KMOD).ko
	sudo insmod $(BIN)/$(KMOD).ko

unload:
	sudo rmmod $(KMOD)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
	rm -f $(BIN)/$(KMOD).ko

.PHONY: all load unload clean

