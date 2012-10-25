/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>

#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"
#include "swap.h"

#define PHYSICAL_OFFSET(x) (((u64)x) & 0xfff)
#define PAGE_IN_USE 1
#define PAGE_NOT_IN_USE 2
#define ERROR_PERMISSION 2
#define NOT_VALID_RANGE 1
#define ALLOCATED_ADDRESS_RANGE 2

struct mem_map * petmem_init_process(void) {
	struct mem_map * new_proc;
	struct vaddr_reg * first_node = (struct vaddr_reg *) kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	new_proc = (struct mem_map *)kmalloc(sizeof(struct mem_map), GFP_KERNEL);
	INIT_LIST_HEAD(&(new_proc->memory_allocations));
	first_node->status = FREE;
	first_node->size = ((PETMEM_REGION_END - PETMEM_REGION_START) >> PAGE_POWER_4KB);
	first_node->page_addr = PETMEM_REGION_START;

	INIT_LIST_HEAD(&(first_node->list));
	list_add(&(first_node->list), &(new_proc->memory_allocations));
    return new_proc;

}

void petmem_deinit_process(struct mem_map * map) {
	struct list_head * pos, * next;
	struct vaddr_reg *entry;
	list_for_each_safe(pos, next, &(map->memory_allocations)){
		entry = list_entry(pos, struct vaddr_reg, list);
        attempt_free_physical_address(entry->page_addr);
		list_del(pos);
		kfree(entry);
	}
	kfree(map);

}

uintptr_t petmem_alloc_vspace(struct mem_map * map, u64 num_pages) {
    printk("Memory allocation\n");
    return allocate(&(map->memory_allocations), num_pages);
}

void petmem_dump_vspace(struct mem_map * map) {
}

// Only the PML needs to stay, everything else can be freed
void petmem_free_vspace(struct mem_map * map, uintptr_t vaddr) {
    printk("Free memory\n");
	free_address(&(map->memory_allocations), vaddr);
    return;

}

/*
 * petmem_free_pspace
 * descrip: Removes any real physical backings to an address.
 *
 */
void petmem_free_pspace(uintptr_t vaddr){

}

/*
 * Though this does use pte64_t, it works with
 * all types of 64, but there is no general one.
 */
void handle_table_memory(void * mem){
    uintptr_t temp;
    pte64_t * handle = (pte64_t *)mem;
	temp = (uintptr_t)__va(petmem_alloc_pages(1));
    printk("Allocated virtual memory is: 0x%012lx, and its physical memory is:0x%012lx\n", temp, __pa(temp));
   //screw it, other way didn't copy permissions, must set them!
    memset((void *)temp, 0, 512*8);
	handle->present = 1;
	handle->page_base_addr = PAGE_TO_BASE_ADDR( __pa(temp ));
}

int petmem_handle_pagefault(struct mem_map * map, uintptr_t fault_addr, u32 error_code) {
	pml4e64_t * cr3;
	pdpe64_t * pdp;
	pde64_t * pde;
	pte64_t * pte;

    int valid_range = check_address_range(map, fault_addr);
    if(valid_range == NOT_VALID_RANGE|| error_code == ERROR_PERMISSION ){
        return -1;
    }

    cr3 = (pml4e64_t *) (CR3_TO_PML4E64_VA( get_cr3() ) + PML4E64_INDEX( fault_addr ) * 8);
    if(!cr3->present) {
        handle_table_memory((void *)cr3);
    }

    pdp = (pdpe64_t *)__va( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr ) + (PDPE64_INDEX( fault_addr ) * 8)) ;
    if(!pdp->present) {
        handle_table_memory((void *) pdp);
        pdp->present = 1;
        pdp->writable = 1;
        pdp->user_page = 1;

    }

    pde = (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ) + PDE64_INDEX( fault_addr )* 8);
    if(!pde->present) {
        handle_table_memory((void *) pde);
        pde->present = 1;
        pde->writable = 1;
        pde->user_page = 1;
    }

    pte = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ) + PTE64_INDEX( fault_addr ) * 8 );

    if(!pte->present) {
        handle_table_memory((void *) pte);
        pte->present = 1;
        pte->writable = 1;
        pte->user_page =1;
    }
#ifdef DEBUG
    printk("~~~~~~~~~~~~~~~~~~~~~NEW PAGE FAULT!~~~~~~~~~~~~\n");
    printk("Error code: %d\n", error_code);
	printk("Fault Address 0x%012lx\n", fault_addr);

    printk("\nCR3 FULL SUMMARY:\n");
    printk("PML4 offset: %lld\n", PML4E64_INDEX(fault_addr));
    printk("PML4 index: %lld (0x%03x)\n", PML4E64_INDEX(fault_addr) * 8, (int)PML4E64_INDEX(fault_addr) * 8);
    printk("The Physical Address:0x%012llx\n", (CR3_TO_PML4E64_PA( get_cr3() ) + PML4E64_INDEX( fault_addr) * 8));
    printk("Virtual Address: 0x%012lx\n", (long unsigned int)cr3);
    printk("Page address to next level from cr3 : 0x%012lx\n", (long unsigned int)cr3->pdp_base_addr);
    printk("\nPDP FULL SUMMARY:\n");
    printk("PDP offset: %lld\n", PDPE64_INDEX(fault_addr));
    printk("PDP index: %lld (0x%03x)\n", PDPE64_INDEX(fault_addr) * 8, (int)PDPE64_INDEX(fault_addr) * 8);
    printk("The Physical Address:0x%012llx\n", (BASE_TO_PAGE_ADDR(cr3->pdp_base_addr) + PDPE64_INDEX( fault_addr) * 8));
    printk("Virtual Address: 0x%012lx\n", (long unsigned int)pdp);
    printk("Page address to next level from pdp : 0x%012lx\n", (long unsigned int)pdp->pd_base_addr);
    printk("\nPDE FULL SUMMARY:\n");
    printk("PDE offset: %lld\n", PDE64_INDEX(fault_addr));
    printk("PDE index: %lld (0x%03x)\n", PDE64_INDEX(fault_addr) * 8, (int)PDE64_INDEX(fault_addr) * 8);
    printk("Hmmm...what is the PDE index? %lld\n", PDE64_INDEX(fault_addr) * 8);
    printk("The Physical Address:0x%012llx\n", (BASE_TO_PAGE_ADDR(pdp->pd_base_addr) + PDE64_INDEX( fault_addr) * 8));   printk("Virtual Address: 0x%012lx\n", (long unsigned int)pde);
    printk("Page address to next level from pde : 0x%012lx\n", (long unsigned int)pde->pt_base_addr);
    printk("\nPTE FULL SUMMARY:\n");
    printk("PTE offset: %lld\n", PTE64_INDEX(fault_addr));
    printk("PTE index: %lld (0x%03x)\n", PTE64_INDEX(fault_addr) * 8, (int)PTE64_INDEX(fault_addr) * 8);
    printk("The Physical Address:0x%012llx\n", (BASE_TO_PAGE_ADDR(pde->pt_base_addr) + PTE64_INDEX( fault_addr) * 8));
    printk("Virtual Address: 0x%012lx\n", (long unsigned int)pte);
    printk("Memory at this : 0x%012lx\n", (long unsigned int)pte->page_base_addr);
#endif
    return 0;
}


void attempt_free_physical_address(uintptr_t address){
 	pml4e64_t * cr3;
	pdpe64_t * pdp, *pdp_table;
	pde64_t * pde, *pde_table;
	pte64_t * pte, *pte_table;
    void * actual_mem;
    /* If one of them isn't there, we don't need to free any physical address because there is none.*/
    cr3 = (pml4e64_t *) (CR3_TO_PML4E64_VA( get_cr3() ) + PML4E64_INDEX( address ) * 8);
    if(!cr3->present) {
        return;
    }
    pdp = (pdpe64_t *)__va( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr ) + (PDPE64_INDEX( address ) * 8)) ;
    pdp_table = (pdpe64_t *)__va( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr )) ;
    if(!pdp->present) {
        return;
    }

    pde = (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ) + PDE64_INDEX( address )* 8);
    pde_table = (pde64_t *)__va(BASE_TO_PAGE_ADDR( pdp->pd_base_addr ));
    if(!pde->present) {
        return;
    }

    pte = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ) + PTE64_INDEX( address ) * 8 );
    pte_table = (pte64_t *)__va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ));
    if(!pte->present) {
        return;
    }

    actual_mem = (void *)__va( BASE_TO_PAGE_ADDR( pte->page_base_addr ) + PHYSICAL_OFFSET( address ) );
    petmem_free_pages((uintptr_t)actual_mem, 1);
    // Chain free this stuff.
    pte->writable = 0;
    pte->user_page = 0;
    pte->present = 0;
    pte->page_base_addr = 0;
    if(is_entire_page_free((void *)pte_table) == PAGE_NOT_IN_USE){
       petmem_free_pages((uintptr_t)pte_table, 1);
    }
    else{
        return;
    }
    pde->writable = 0;
    pde->user_page = 0;
    pde->present = 0;
    pde->pt_base_addr = 0;
    if(is_entire_page_free((void *)pde_table) == PAGE_NOT_IN_USE){
       petmem_free_pages((uintptr_t)pde_table, 1);
    }
    else{
        return;
    }
    pdp->writable = 0;
    pdp->user_page = 0;
    pdp->present = 0;
    pdp->pd_base_addr = 0;
    if(is_entire_page_free((void *)pdp_table) == PAGE_NOT_IN_USE){
       petmem_free_pages((uintptr_t)pdp_table, 1);
    }
    else{
        return;
    }
    cr3->present = 0;
    cr3->pdp_base_addr = 0;
    cr3->user_page = 0;
    cr3->writable = 0;

}
int is_entire_page_free(void * page_structure){
    int i = 0;
    for(i = 0; i < 512; i++){
        int offset = i * 8;
        pte64_t * page = (page_structure + offset);
        if(page->present == 1){
            return PAGE_IN_USE;
        }
    }
    return PAGE_NOT_IN_USE;
}

void free_address(struct list_head * head_list, u64 page){
	struct vaddr_reg * cur, * found, *next, *prev;
	found = NULL;
	list_for_each_entry(cur ,head_list, list){
		if(cur->page_addr == page){
		    found = cur;
        }
	}
	if(found == NULL){
		return;
	}
	//TODO: Remove actually allocated pages here.
    attempt_free_physical_address(found->page_addr);
	//Set the clear values.
	found->status = FREE;


	//Coalesce nodes.
	next = list_entry(found->list.next, struct vaddr_reg, list);
	prev = list_entry(found->list.prev, struct vaddr_reg, list);

	if(next->page_addr != page && next->status == FREE){
		list_del(found->list.next);
		found->size += next->size;
		kfree(next);
	}
	if(prev->page_addr != page && prev->status == FREE){
        found->page_addr = prev->page_addr;
		list_del(found->list.prev);
		found->size += prev->size;
		kfree(prev);
	}

}

uintptr_t  allocate(struct list_head * head_list, u64 size){
	struct vaddr_reg *cur, *node_to_consume, *new_node;
	u64 current_size;
	node_to_consume = NULL;
	list_for_each_entry(cur, head_list, list){
		if(cur->status == FREE && cur->size >= size){
			node_to_consume = cur;
			break;
		}
	}
	if(node_to_consume == NULL){
		return -1;
	}

	printk("Node to break apart: %p\n", (void *)node_to_consume->page_addr);
	node_to_consume->status = ALLOCATED;
	if(node_to_consume->size == size){
		return node_to_consume->page_addr;
	}

	current_size = node_to_consume->size;
	current_size -= size;
	new_node = (struct vaddr_reg*)kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	INIT_LIST_HEAD(&(new_node->list));
	list_add(&(new_node->list), &(node_to_consume->list));
	new_node->size = current_size;
	new_node->page_addr = node_to_consume->page_addr + (size << PAGE_POWER_4KB);
	new_node->status = FREE;

	node_to_consume->size = size;
	return node_to_consume->page_addr;
}

void print_bits(u64 * num){
    u64 current_bit = 1;
    int i = 0;
    for(i = 0; i < 64; i++){
        printk("%1d", (int)((*num) & current_bit) >> (i));
        current_bit = current_bit << 1;
    }
}
int check_address_range(struct mem_map * map, uintptr_t address){
 	struct vaddr_reg * cur;
	list_for_each_entry(cur, &(map->memory_allocations), list){
        if(cur->status == ALLOCATED && address >= cur->page_addr && address < (cur->page_addr + 4096 * cur->size)){
            return ALLOCATED_ADDRESS_RANGE;
        }
	}
    return NOT_VALID_RANGE;

}
