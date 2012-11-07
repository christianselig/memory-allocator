/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#ifndef __ON_DEMAND_H__
#define __ON_DEMAND_H__


#include <linux/module.h>
#include <linux/list.h>
#include "swap.h"
#define ALLOCATED 0
#define PHYSICALLY_ALLOCATED 1
struct vaddr_reg {
   /* You can use this to demarcate virtual address allocations */
	u8 status;
	u64 size;
	u64 page_addr;
	struct list_head list;
};

struct mem_map {
   /* Add your own state here */
	struct list_head memory_allocations;
    struct list_head * clock_hand;
};




struct mem_map * petmem_init_process(void);
void petmem_deinit_process(struct mem_map * map);

uintptr_t petmem_alloc_vspace(struct mem_map * map, u64 num_pages);
void petmem_free_vspace(struct mem_map * map, uintptr_t vaddr);

int handle_table_memory(void * mem, struct mem_map * map);
void petmem_dump_vspace(struct mem_map * map);

//Put page in the void *, return -1 if the page is not valid (FREE or not allocated).
uintptr_t get_valid_page_entry(uintptr_t address);
void * page_replacement_clock(struct mem_map * map);
// How do we get error codes??
int petmem_handle_pagefault(struct mem_map * map, uintptr_t fault_addr, u32 error_code);
void print_bits(u64* num);
void free_address(struct list_head * head_list, u64 page);
void attempt_free_physical_address(uintptr_t address);
int is_entire_page_free(void * page_structure);
int check_address_range(struct mem_map * map, uintptr_t address);
uintptr_t allocate(struct list_head * head_list, u64 size);
#endif
