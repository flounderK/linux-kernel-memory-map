#!/bin/bash

mkdir -p rootfs

if [ $(id -u) -ne 0 ]; then
	echo "This script must be run as root or with mount privileges"
fi

mount rootfs.ext2 rootfs
# get the path of the
ROOT_MODULES_DIRPATH=$(find rootfs/lib/modules/ -mindepth 1 -maxdepth 1 -type d)
find src -type f -iname '*.ko' -o -iname '*.mod' | xargs -I{} sh -c "cp {} ${ROOT_MODULES_DIRPATH}"

# insert install command into boot scripts to be executed by rootfs/etc/init.d/rcS

if [ -z "$(grep 'memory_map.ko:' ${ROOT_MODULES_DIRPATH}/modules.dep)" ]; then
	echo "adding entry to modules.dep"
	echo 'memory_map.ko:' >>  ${ROOT_MODULES_DIRPATH}/modules.dep
fi

INSTALL_MODULE_SCRIPT_PATH="rootfs/etc/init.d/S41install_module"

cat <<-_EOF_ > "${INSTALL_MODULE_SCRIPT_PATH}"
    #!/bin/sh
	# install to rootfs/etc/init.d/
	/sbin/modprobe memory_map.ko
	mknod /dev/kmaps c 251 0
_EOF_

chmod +x "${INSTALL_MODULE_SCRIPT_PATH}"


if [ -e "testsrc/arb_read" ]; then
	cp testsrc/arb_read rootfs/root/arb_read
fi

# cp scripts/install_module_cmd.sh rootfs/etc/init.d/

echo "unmounting rootfs"
UMOUNT_RETRY=0
UMOUNT_MAX_RETRY=3
while [ $UMOUNT_RETRY -ne $UMOUNT_MAX_RETRY ]; do
	echo "umount retry $UMOUNT_RETRY"
	umount rootfs
	LASTERR=$?
	echo "lasterr $LASTERR"
	if [ $LASTERR -eq 0 ]; then
		break
	fi
	sleep 3
	UMOUNT_RETRY=$(($UMOUNT_RETRY + 1))
done
