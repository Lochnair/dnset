dnset-y := main.o storage.o
obj-m := dnset.o
KERNEL_VERSION := $(shell uname -r)
IDIR := /lib/modules/$(KERNEL_VERSION)/kernel/net/
KDIR := /lib/modules/$(KERNEL_VERSION)/build
PWD := $(shell pwd)
VERSION := $(shell git rev-parse HEAD 2>/dev/null)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

install:
	install -v -m 644 dnset.ko $(IDIR)
	depmod "$(KERNEL_VERSION)"
	[ "$(KERNEL_VERSION)" != `uname -r` ] || modprobe dnset

clean:
	rm -rf Module.markers modules.order Module.symvers *.o *.mod.c *.mod.o *.ko .*.o.cmd .*.ko.cmd
