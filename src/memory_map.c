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
        //
        snprintf(out_buffer, sizeof(out_buffer),
                KERN_INFO "%016lx: %s %s",
                ((size_t)data) + i,
                hex_char_buf,
                ascii
                );

        printk(out_buffer);




    }

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clifton Wolfe");
MODULE_DESCRIPTION("Memory map of the kernel");
static atomic_t memory_map_open_count;
static int major_num = 0;
static struct mm_struct *current_mm = NULL;

/* When a process reads from our device, this gets called. */
static ssize_t memory_map_read(struct file *flip, char *buffer, size_t len, loff_t *offset) {
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
    dmesg_dump_hex(addr, 256);


    return nread;
}
/* Called when a process tries to write to our device */
static ssize_t memory_map_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) {
    /* This is a read-only device */
    printk(KERN_ALERT "This operation is not supported.\n");
    return -EINVAL;
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
    atomic_dec(&memory_map_open_count);
    module_put(THIS_MODULE);
    return 0;
}

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
    .read = memory_map_read,
    .write = memory_map_write,
    .open = memory_map_open,
    .release = memory_map_release
};

static int __init memory_map_init(void)
{
    printk(KERN_INFO "Starting kernel module!\n");
    // TODO: real major num here
    int res = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (res < 0) {
        printk(KERN_WARNING "Failed to start kernel module %d\n", res);
        return res;
    }
    printk(KERN_INFO "Major number %d\n", res);

    major_num = res;
    return 0;
}

static void __exit memory_map_cleanup(void)
{
    printk(KERN_INFO "Cleaning up module.\n");
    unregister_chrdev(major_num, DEVICE_NAME);
}

module_init(memory_map_init);
module_exit(memory_map_cleanup);
