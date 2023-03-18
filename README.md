


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
``
