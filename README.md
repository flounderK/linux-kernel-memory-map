# Overview
I want to be able to see what's in memory damnit.
This is a kernel module that is meant to eventually write out a nice formatted output showing what is where in kernel space *without* having to hook/overwrite kmalloc and kfree. For right now though, it just lets you read and write to arbitrary virtual and physical addresses.


## Build the linux kernel
```
export KBUILD_OUTPUT="$(pwd)/build"
make mrproper
make defconfig
make
export MODULE_DIR="$KBUILD_OUTPUT"
unset KBUILD_OUTPUT
```

## Build the linux kernel using buildroot
```
make qemu_x86_64_defconfig
make
```


## Building
```
# export KDIR=<path-to-linux-kernel-build-dir>
export KDIR=<buildroot-path>/output/build/linux-<kernel-version>
make
```

## Start test environment
```
# cp $KDIR/arch/x86/boot/bzImage .
cp buildroot/output/images/* .
sudo ./script/test_install.sh
./start-qemu.sh
```

## Testing
### Physical memory reading
```
cat /proc/iomem
./arb_read -a <Kernel code physical address> -p -n 512| xxd
```



