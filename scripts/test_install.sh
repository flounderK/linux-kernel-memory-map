#!/bin/bash

mkdir -p rootfs
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


if [ -e "testsrc/main" ]; then
	cp testsrc/main rootfs/root/main
fi

# cp scripts/install_module_cmd.sh rootfs/etc/init.d/

umount rootfs
