


## Build the linux kernel
```
export KBUILD_OUTPUT="$(pwd)/build"
make mrproper
make defconfig
make
export MODULE_DIR="$KBUILD_OUTPUT"
unset KBUILD_OUTPUT
```


```
mkdir rootfs
cd rootfs && find . -print | cpio -o -H newc > ../rootfs.cpio && gzip ../rootfs.cpio

```

```
cp $MODULE_DIR/arch/x86/boot/bzImage .
```
