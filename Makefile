LINUX_KERN=/usr/src/kernels/2.6.38.6-26.rc1.fc15.x86_64

EXTRA_CFLAGS  += -DMODULE=1 -D__KERNEL__=1


petmem-y := 	main.o \
		swap.o \
		buddy.o \
		file_io.o \
		on_demand.o 

petmem-objs := $(petmem-y)
obj-m := petmem.o


all:
	$(MAKE) -C $(LINUX_KERN) M=$(PWD) modules

clean:
	$(MAKE) -Wformat -C $(LINUX_KERN) M=$(PWD) clean

