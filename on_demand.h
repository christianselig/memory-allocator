/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#ifndef __ON_DEMAND_H__
#define __ON_DEMAND_H__


#include <linux/module.h>
#include <linux/list.h>

#include "swap.h"

struct vaddr_reg {
	//	Use the kernel's linked list type
	struct list_head list;
	//	Region start address
	u64 start;
	//	The length of this region
	u64 length;
	//	Is the region free or not
	char free;
};

struct mem_map {
	struct vaddr_reg* memory;
};



struct mem_map * petmem_init_process(void);
void petmem_deinit_process(struct mem_map * map);

uintptr_t petmem_alloc_vspace(struct mem_map * map, u64 num_pages);
void petmem_free_vspace(struct mem_map * map, uintptr_t vaddr);

void petmem_dump_vspace(struct mem_map * map);

// How do we get error codes??
int petmem_handle_pagefault(struct mem_map * map, uintptr_t fault_addr, u32 error_code);

#endif
