obj-m += memory_map.o
# to stop gcc from using c90 for whatever reason
ccflags-y := -std=gnu99 -Wno-declaration-after-statement -Wno-unused-variable

KDIR?=/lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
