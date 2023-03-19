#ifndef __MEMORY_MAP_H__
#define __MEMORY_MAP_H__
#include <linux/ioctl.h>

#define MEMORY_MAP_IOCTL_TYPE 0x86
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)


enum memory_map_access_type {
    ACCESS_MODE_VIRT = 0,
    ACCESS_MODE_PHYS
};


#define MEMORY_MAP_IOCTL_ACCESS_MODE_SET \
    _IOW(MEMORY_MAP_IOCTL_TYPE, 0, int32_t)


#define MEMORY_MAP_IOCTL_ACCESS_MODE_GET \
    _IOR(MEMORY_MAP_IOCTL_TYPE, 1, int32_t)

#endif //ifndef __MEMORY_MAP_H__
