obj-m += memory_map.o

MODULE_DIR?=/lib/modules/$(shell uname -r)/build

all:
	make -C $(MODULE_DIR) M=$(PWD) modules

clean:
	make -C $(MODULE_DIR) M=$(PWD) clean
