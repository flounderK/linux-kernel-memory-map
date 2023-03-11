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

#define MODULE_NAME "memory_map"
#define DEVICE_NAME "kmaps"

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
    size_t nread = 0;

    vm_area = mm->mmap;

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
    major_num = printk(KERN_INFO "Starting kernel module!\n");
    int res = register_chrdev(0, DEVICE_NAME, &file_ops);
    if (res < 0) {
        printk(KERN_WARNING "Failed to start kernel module %d\n", res);
    }
    return res;
}

static void __exit memory_map_cleanup(void)
{
    printk(KERN_INFO "Cleaning up module.\n");
    unregister_chrdev(major_num, DEVICE_NAME);
}

module_init(memory_map_init);
module_exit(memory_map_cleanup);
