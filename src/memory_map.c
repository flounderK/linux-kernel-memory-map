#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/mm_types.h>  // for mm_struct and vm_area_struct
#include <linux/sched/mm.h>
#include <asm/page.h>
#include <linux/pgtable.h>
#include <linux/slab.h>                 //kmalloc()
#include <linux/ioctl.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/ioport.h>  // for iomem_resource and resource functions
#include <linux/device.h>  // for class_create
#include "memory_map.h"

// NOTE: `/proc/iomem`'s code can be found mostly in `kernel/resource.c`
// walk_mem_res and walk_iomem_res_desc are the more important functions

#define MODULE_NAME "memory_map"
#define DEVICE_NAME "kmaps"

void DumpHex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printk(KERN_INFO "%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printk(KERN_INFO " ");
			if ((i+1) % 16 == 0) {
				printk(KERN_INFO "|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printk(KERN_INFO " ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printk(KERN_INFO "   ");
				}
				printk(KERN_INFO "|  %s \n", ascii);
			}
		}
	}
}

void dmesg_dump_hex(const void* data, size_t size) {
    char out_buffer[128];
    char hex_char_buf[64];
    uint8_t byte_chr;
	char ascii[17];
    memset(ascii, 0, sizeof(ascii));
    ascii[16] = '\0';
    for (int i = 0; i < size; i = i + (sizeof(void*)*2)){
        memset(out_buffer, 0, sizeof(out_buffer));
        memset(hex_char_buf, 0, sizeof(hex_char_buf));
        for (int c = 0; c < 16; c++) {
            byte_chr = *(unsigned char*)((size_t)data+i+c);
            if (byte_chr >= ' ' && byte_chr <= '~') {
                ascii[c % 16] = byte_chr;
            } else {
                ascii[c % 16] = '.';
            }
        }
        // TODO: maybe make this less bad
        //
        snprintf(hex_char_buf, sizeof(hex_char_buf),
                "%02x%02x %02x%02x "
                "%02x%02x %02x%02x "
                "%02x%02x %02x%02x "
                "%02x%02x %02x%02x ",
                *(uint8_t*)((size_t)data+i+0),
                *(uint8_t*)((size_t)data+i+1),
                *(uint8_t*)((size_t)data+i+2),
                *(uint8_t*)((size_t)data+i+3),

                *(uint8_t*)((size_t)data+i+4),
                *(uint8_t*)((size_t)data+i+5),
                *(uint8_t*)((size_t)data+i+6),
                *(uint8_t*)((size_t)data+i+7),

                *(uint8_t*)((size_t)data+i+8),
                *(uint8_t*)((size_t)data+i+9),
                *(uint8_t*)((size_t)data+i+10),
                *(uint8_t*)((size_t)data+i+11),

                *(uint8_t*)((size_t)data+i+12),
                *(uint8_t*)((size_t)data+i+13),
                *(uint8_t*)((size_t)data+i+14),
                *(uint8_t*)((size_t)data+i+15)
                );
        /*
        snprintf(out_buffer, sizeof(out_buffer),
                KERN_INFO "%016lx: %s %s",
                ((size_t)data) + i,
                hex_char_buf,
                ascii
                );
                */

        printk(KERN_INFO "%016lx: %s %s",
                ((size_t)data) + i,
                hex_char_buf,
                ascii);
    }
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clifton Wolfe");
MODULE_DESCRIPTION("Memory map of the kernel");
static atomic_t memory_map_open_count;
static int major_num = 0;
static struct class* pClass = NULL;
static dev_t dev_no;
static struct mm_struct *current_mm = NULL;
// TODO: it might be a good idea to make this atomic
static size_t current_addr = 0;
static int32_t memory_access_mode = ACCESS_MODE_VIRT;

static ssize_t arbitrary_virtual_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {

    ssize_t nread = 0;
    void* from_addr = (void*)current_addr;//*(void**)offset;
    printk(KERN_INFO "Read offset 0x%lx\n", (size_t)from_addr);
    void* kbuf = kmalloc(len, GFP_USER);
    if (kbuf == NULL) {
        printk(KERN_INFO "Failed to allocate memory for copy");
        nread = -ENOMEM;
        goto exit;
    }
    memcpy(kbuf, from_addr, len);

    unsigned long failed_to_copy = copy_to_user(buffer, kbuf, len);
    if (failed_to_copy > 0) {
        printk(KERN_INFO "failed to copy %lu\n", failed_to_copy);
        nread = -EFAULT;
        goto exit;
    }
    nread = len;

    // *offset += nread;
    current_addr += nread;

exit:
    if (kbuf != NULL) {
        kfree(kbuf);
        kbuf = NULL;
    }
    return nread;
}

static ssize_t arbitrary_physical_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {

    ssize_t nread = 0;
    volatile size_t __iomem * from_addr = NULL;
    //void* from_addr = (void*)current_addr;//*(void**)offset;
    printk(KERN_INFO "Read offset 0x%lx\n", (size_t)current_addr);
    size_t* kbuf = kmalloc(len, GFP_USER);
    if (kbuf == NULL) {
        printk(KERN_INFO "Failed to allocate memory for copy");
        nread = -ENOMEM;
        goto exit;
    }
    memset(kbuf, 0, len);
    //from_addr = ioremap(current_addr, len);
    from_addr = memremap(current_addr, len, MEMREMAP_WB);
    if (from_addr == NULL) {
        printk(KERN_INFO "ioremap failed\n");
        goto exit;
    }
    for (int i = 0; i < len/sizeof(u64); i++) {
        //printk(KERN_INFO "reading from 0x%lx\n", &from_addr[i]);
        //kbuf[i] = readq((void*)((size_t)from_addr + (size_t)i));
        kbuf[i] = readq(&from_addr[i]);
    }
    size_t qword_aligned_len = ((len/sizeof(u64))*sizeof(u64));
    // if the len isn't qword aligned, copy the last few bytes
    for (size_t i = qword_aligned_len+sizeof(u64); i < len; i++) {
        *(u8*)((size_t)kbuf + i) = readb((void*)((size_t)from_addr + i));
    }


    //memcpy(kbuf, from_addr, len);

    unsigned long failed_to_copy = copy_to_user(buffer, kbuf, len);
    if (failed_to_copy > 0) {
        printk(KERN_INFO "failed to copy %lu\n", failed_to_copy);
        nread = -EFAULT;
        goto exit;
    }
    nread = len;

    // *offset += nread;
    current_addr += nread;

exit:
    if (kbuf != NULL) {
        kfree(kbuf);
        kbuf = NULL;
    }
    if (from_addr != NULL) {
        //iounmap(from_addr);
        memunmap(from_addr);
        from_addr = NULL;
    }
    return nread;
}


static ssize_t arbitrary_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {
    int access_mode = memory_access_mode;
    if (access_mode == ACCESS_MODE_VIRT) {
        return arbitrary_virtual_read(flip, buffer, len, offset);
    }
    else if (access_mode == ACCESS_MODE_PHYS) {
        return arbitrary_physical_read(flip, buffer, len, offset);
    }
    else {
        return -EBUSY;
    }

}

static int ioctl_debug_stuff(struct file *file, unsigned int cmd, unsigned long arg){
    struct rb_node *rb;
    struct vma *vma;
    struct vm_area_struct* vm_area;
    struct page* page;
    pgd_t * pgd = NULL;
    pmd_t * pmd = NULL;
    pte_t * pte = NULL;
    size_t nread = 0;
    size_t addr = (size_t)&try_module_get;
    printk(KERN_INFO "try_module_get 0x%lx\n", addr);
    size_t base = addr - 0xc1720;
    printk(KERN_INFO "base 0x%lx\n", base);
    printk(KERN_INFO "current_addr 0x%lx\n", current_addr);
    size_t pgdir_mask = PGDIR_MASK;
    printk(KERN_INFO "PGDIR_MASK 0x%lx\n", pgdir_mask);

    size_t pmd_mask = PMD_MASK;
    printk(KERN_INFO "PMD_MASK 0x%lx\n", pmd_mask);

    pgd = current_mm->pgd;

    printk(KERN_INFO "current_mm 0x%lx\n", (size_t)current_mm);
    printk(KERN_INFO "current_mm base 0x%lx\n", (size_t)current_mm->mmap_base);

    printk(KERN_INFO "pgd ptr 0x%lx\n", (size_t)pgd);
    printk(KERN_INFO "pgd 0x%lx\n", *(size_t*)pgd);

    //DumpHex(pgd, 256);
    dmesg_dump_hex(base, 256);
    return 0;
}
/* Called when a process tries to write to our device */
static ssize_t arbitrary_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {

    ssize_t nwritten = 0;
    void* to_addr = (void*)current_addr;//*(void**)offset;
    printk(KERN_INFO "Write offset 0x%lx\n", (size_t)to_addr);
    void* kbuf = kmalloc(len, GFP_USER);
    if (kbuf == NULL) {
        printk(KERN_INFO "Failed to allocate memory for copy");
        nwritten = -ENOMEM;
        goto exit;
    }

    unsigned long failed_to_copy = copy_from_user(kbuf, buffer, len);
    if (failed_to_copy > 0) {
        printk(KERN_INFO "failed to copy %lu\n", failed_to_copy);
        nwritten = -EFAULT;
        goto exit;
    }

    memcpy(to_addr, kbuf, len);
    nwritten = len;

    // *offset += nwritten;
    current_addr += nwritten;

exit:
    if (kbuf != NULL) {
        kfree(kbuf);
        kbuf = NULL;
    }
    return nwritten;
}

static loff_t arbitrary_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t res = 0;
    if (whence == SEEK_SET) {
        // current_addr = offset;
        printk(KERN_INFO "setting offset to 0x%llx\n", offset);
        res = offset;
    }
    else if (whence == SEEK_CUR) {
        res = current_addr + offset;
        //res = file->f_pos + offset;
    }
    else {
        res = -EINVAL;
        goto exit;
    }
    //file->f_pos = res;
    current_addr = res;

exit:
    return res;
}


/* Called when a process opens our device */
static int memory_map_open(struct inode *inode, struct file *file) {
    /* If device is open, return busy */
    if (atomic_read(&memory_map_open_count)) {
        return -EBUSY;
    }
    printk(KERN_INFO "opening device\n");
    atomic_inc(&memory_map_open_count);
    current_mm = current->mm;
    try_module_get(THIS_MODULE);
    return 0;
}
/* Called when a process closes our device */
static int memory_map_release(struct inode *inode, struct file *file) {
    /* Decrement the open counter and usage count. Without this, the module would not unload. */
    current_mm = NULL;
    current_addr = 0;
    memory_access_mode = ACCESS_MODE_VIRT;
    atomic_dec(&memory_map_open_count);
    module_put(THIS_MODULE);
    return 0;
}

static long memory_map_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case MEMORY_MAP_IOCTL_ACCESS_MODE_SET:
            printk(KERN_INFO "setting access mode to %ld\n", arg);
            memory_access_mode = arg;
            break;
        case MEMORY_MAP_IOCTL_ACCESS_MODE_GET:
            return memory_access_mode;
            break;
        default:
            printk(KERN_INFO "Default\n");
            break;
    }
    return 0;
}

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
    .read = arbitrary_read,
    .write = arbitrary_write,
    .open = memory_map_open,
    .release = memory_map_release,
    .llseek = arbitrary_llseek,
    .unlocked_ioctl = memory_map_ioctl
};

static int __init memory_map_init(void)
{
    struct device* pDev = NULL;
    printk(KERN_INFO "Starting kernel module!\n");
    int res = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (res < 0) {
        printk(KERN_WARNING "Failed to start kernel module %d\n", res);
        return res;
    }
    printk(KERN_INFO "Major number %d\n", res);

    major_num = res;

    dev_no = MKDEV(major_num, 0);

    pClass = class_create(THIS_MODULE, MODULE_NAME);
    if (IS_ERR(pClass)) {
        printk(KERN_WARNING "can't create class\n");
        unregister_chrdev_region(dev_no, 1);
        return -1;
    }
    pDev = device_create(pClass, NULL, dev_no, NULL, DEVICE_NAME);
    if (IS_ERR(pDev)) {
        printk(KERN_WARNING "can't create device /dev/" DEVICE_NAME "\n");
        class_destroy(pClass);
        unregister_chrdev_region(dev_no, 1);
        return -1;
    }

    printk(KERN_INFO MODULE_NAME " loaded\n");
    return 0;
}

static void __exit memory_map_cleanup(void)
{
    printk(KERN_INFO "Cleaning up module.\n");
    device_destroy(pClass, dev_no); // remote /dev node
    class_destroy(pClass); // delete class from /sys/class
    unregister_chrdev(major_num, DEVICE_NAME);
}

module_init(memory_map_init);
module_exit(memory_map_cleanup);
