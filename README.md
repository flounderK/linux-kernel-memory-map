


## Build the linux kernel
```
export KBUILD_OUTPUT="$(pwd)/build"
make mrproper
make defconfig
make
export MODULE_DIR="$KBUILD_OUTPUT"
unset KBUILD_OUTPUT
```


## building
```
cd src
export KDIR=<path-to-linux-kernel-build-dir>
make
cd ..
```

## start test environment
```
cp $KDIR/arch/x86/boot/bzImage .
sudo -sE
./script/test_install.sh
exit
./start-qemu.sh
``
