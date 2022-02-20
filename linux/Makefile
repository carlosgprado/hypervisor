ifeq ($(KERNELRELEASE),)
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules EXTRA_CFLAGS="-g -DDEBUG"

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

main:
	$(CC) -o main -ggdb main.c

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod *.order *.symvers
	-rm main

.PHONY: modules modules_install clean

else
	# Called from kernel build system: just declare what our 
	# modules are
	obj-m := hypervisor.o

endif

