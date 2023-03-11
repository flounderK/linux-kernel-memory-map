obj-m += memory_map.o
# to stop gcc from using c90 for whatever reason
ccflags-y := -std=gnu99 -Wno-declaration-after-statement -Wno-unused-variable

MODULE_DIR?=/lib/modules/$(shell uname -r)/build

all:
	make -C $(MODULE_DIR) M=$(PWD) modules

clean:
	make -C $(MODULE_DIR) M=$(PWD) clean
