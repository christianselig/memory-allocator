#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "petmem.h"
#include "buddy.h"
#include "on_demand.h"
#include "pgtables.h"

MODULE_LICENSE("GPL");


struct class * petmem_class = NULL;
static struct cdev ctrl_dev;
static int major_num = 0;


LIST_HEAD(petmem_pool_list); 


uintptr_t petmem_alloc_pages(u64 num_pages) {
    uintptr_t vaddr = 0;
    struct buddy_mempool * tmp_pool = NULL;
    int page_order = get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT;

    // allocate from buddy 
    list_for_each_entry(tmp_pool, &petmem_pool_list, node) {
	// Get allocation size order
	vaddr = (uintptr_t)buddy_alloc(tmp_pool, page_order);
	if (vaddr) break;
    }
    
    if (!vaddr) {
	printk("Failed to allocate %llu pages\n", num_pages);
	return (uintptr_t)NULL;
    }

    return (uintptr_t)__pa(vaddr);
}


void petmem_free_pages(uintptr_t page_addr, u64 num_pages) {
    struct buddy_mempool * tmp_pool = NULL;
    int page_order = get_order(num_pages * PAGE_SIZE) + PAGE_SHIFT;
    uintptr_t page_va = __va(page_addr);

    printk("Freeing %llu pages at %p\n", num_pages, (void *)page_va);

    list_for_each_entry(tmp_pool, &petmem_pool_list, node) {
	if ((page_va >= tmp_pool->base_addr) && 
	    (page_va < tmp_pool->base_addr + (0x1 << tmp_pool->pool_order))) {

	    buddy_free(tmp_pool, page_va, page_order);
	    break;
	}
    }

    return;
}


static long petmem_ioctl(struct file * filp, 
			 unsigned int ioctl, unsigned long arg) {
    void __user * argp = (void __user *)arg;


    printk("petmem ioctl\n");

    switch (ioctl) {
	case ADD_MEMORY: {
	    struct memory_range reg;
	    int reg_order = 0;
	    uintptr_t base_addr = 0;	    
	    u32 num_pages = 0;

	    if (copy_from_user(&reg, argp, sizeof(struct memory_range))) {
		printk("Error copying memory region from user space\n");
		return -EFAULT;
	    };


	    base_addr = (uintptr_t)__va(reg.base_addr);
	    num_pages = reg.pages;

	    // The order is equal to the highest order bit
	    for (reg_order = fls(num_pages);
		 reg_order != 0;
		 reg_order = fls(num_pages)) {
		struct buddy_mempool * tmp_pool = NULL;
    
		printk("Adding pool of order %d (%d pages) at %p\n", 
		       reg_order + PAGE_SHIFT - 1, num_pages, (void *)base_addr);


		// Page size is the minimum allocation size
		tmp_pool = buddy_init(base_addr, reg_order + PAGE_SHIFT - 1, PAGE_SHIFT); 
		
		if (tmp_pool == NULL) {
		    printk("ERROR: Could not initialize buddy pool for memory at %p (order=%d)\n", 
			   (void *)base_addr, reg_order);
		    break;
		}

		buddy_free(tmp_pool, (void *)base_addr, reg_order + PAGE_SHIFT - 1);

		list_add(&(tmp_pool->node), &petmem_pool_list);

		num_pages -= (0x1 << (reg_order - 1));
		base_addr += ((0x1 << (reg_order - 1)) * PAGE_SIZE);
	    }

	    break;
	}



	case LAZY_ALLOC: {
	    struct alloc_request req;
	    struct mem_map * map = filp->private_data;
	    u64 page_size = 0;
	    u64 num_pages = 0;

	    memset(&req, 0, sizeof(struct alloc_request));

	    if (copy_from_user(&req, argp, sizeof(struct alloc_request))) {
		printk("Error copying allocation request from user space\n");
		return -EFAULT;
	    }

	    printk("Requested allocation of %llu bytes\n", req.size);

	    page_size = (req.size + (PAGE_SIZE - 1)) & (~(PAGE_SIZE - 1));
	    num_pages = page_size >> PAGE_SHIFT;

	    req.addr = petmem_alloc_vspace(map, num_pages);
	    
	    if (req.addr == 0) {
		printk("Error: Could not allocate virtual address region\n");
		return 0;
	    }

	    if (copy_to_user(argp, &req, sizeof(struct alloc_request))) {
		printk("Error copying allocation request to user space\n");
		return -EFAULT;
	    }

	    break;
	}

	case LAZY_FREE: {
	    uintptr_t addr = (uintptr_t)arg;
	    struct mem_map * map = filp->private_data;

	    petmem_free_vspace(map, addr);

	    break;

	}

	case LAZY_DUMP_STATE: {
	    struct mem_map * map = filp->private_data;

	    petmem_dump_vspace(map);
	    break;
	}
	    
	case PAGE_FAULT: {
	    struct page_fault fault;
	    struct mem_map * map = filp->private_data;

	    memset(&fault, 0, sizeof(struct page_fault));

	    if (copy_from_user(&fault, argp, sizeof(struct page_fault))) {
		printk("Error copying page fault info from user space\n");
		return -EFAULT;
	    }
	    
	    if (petmem_handle_pagefault(map, (uintptr_t)fault.fault_addr, (u32)fault.error_code) != 0) {
		printk("error handling page fault for Addr:%p (error=%d)\n", (void *)fault.fault_addr, fault.error_code);
		return 1;
	    }



	    // 0 == success
	    return 0;
	}

	case INVALIDATE_PAGE: {
	    uintptr_t addr = (uintptr_t)arg;
	    invlpg(PAGE_ADDR(addr));
	    break;
	}


	case RELEASE_MEMORY:

	default:
	    printk("Unhandled ioctl (%d)\n", ioctl);
	    break;
    }

    return 0;

}


static int petmem_open(struct inode * inode, struct file * filp) {

    
    filp->private_data = petmem_init_process();
    
    return 0;
}


static int petmem_release(struct inode * inode, struct file * filp) {
    struct mem_map * map = filp->private_data;

    // garbage collect

    petmem_deinit_process(map);

    return 0;
}


static struct file_operations ctrl_fops = {
    .owner = THIS_MODULE,
    .open = petmem_open,
    .release = petmem_release,
    .unlocked_ioctl = petmem_ioctl,
    .compat_ioctl = petmem_ioctl
};
    


static int __init petmem_init(void) {
    dev_t dev = MKDEV(0, 0);
    int ret = 0;

    printk("-------------------------\n");
    printk("-------------------------\n");
    printk("Initializing Pet Memory manager\n");
    printk("-------------------------\n");
    printk("-------------------------\n");


    petmem_class = class_create(THIS_MODULE, "petmem");

    if (IS_ERR(petmem_class)) {
	printk("Failed to register Pet Memory class\n");
	return PTR_ERR(petmem_class);
    }

    ret = alloc_chrdev_region(&dev, 0, 1, "petmem");

    if (ret < 0) {
	printk("Error Registering memory controller device\n");
	class_destroy(petmem_class);
	return ret;
    }


    major_num = MAJOR(dev);
    dev = MKDEV(major_num, 0);

    cdev_init(&ctrl_dev, &ctrl_fops);
    ctrl_dev.owner = THIS_MODULE;
    ctrl_dev.ops = &ctrl_fops; 
    cdev_add(&ctrl_dev, dev, 1);

    
    device_create(petmem_class, NULL, dev, NULL, "petmem");

    return 0;
}


static void __exit petmem_exit(void) {
    dev_t dev = 0;

    dev = MKDEV(major_num, 0);

    unregister_chrdev_region(MKDEV(major_num, 0), 1);
    cdev_del(&ctrl_dev);
    device_destroy(petmem_class, dev);
    
    class_destroy(petmem_class);


    // deinit buddy pools
    //    list_for_each_entry_safe(...)

}





module_init(petmem_init);
module_exit(petmem_exit);
