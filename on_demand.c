/* On demand Paging Implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>

#include "petmem.h"
#include "on_demand.h"
#include "pgtables.h"
#include "swap.h"

int debug = 0;

struct mem_map * petmem_init_process(void) {
	struct mem_map *map = (struct mem_map *)kmalloc(sizeof(struct mem_map), GFP_KERNEL);
	struct vaddr_reg *new;
	if(!map) {
		printk("--> petmem_init_process: Could not kmalloc the memory map!\n");
		return NULL;
	}
	map->memory = (struct vaddr_reg *)kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	if(!map->memory) {
		printk("--> petmem_init_process: Could not kmalloc the beginning pointer!\n");
		return NULL;
	}
	INIT_LIST_HEAD(&map->memory->list);
	map->memory->start = 0;
	map->memory->length = 0;
	map->memory->free = 0;

	new = (struct vaddr_reg *)kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
	if(!new) {
		printk("--> petmem_init_process: Could not kmalloc the first node!\n");
		return NULL;
	}
	INIT_LIST_HEAD(&(new->list));
	new->start = PETMEM_REGION_START;
	new->length = PETMEM_REGION_END - PETMEM_REGION_START;
	new->free= 1;

	list_add(&(new->list), &(map->memory->list));
	/*map->memory->start = PETMEM_REGION_START;
	map->memory->length = PETMEM_REGION_END - PETMEM_REGION_START;
	map->memory->free = 1;
	//printk("--> petmem_init_process: Validating = Start: %lld Length: %lld Free: %d\n", map->memory->start, map->memory->length, map->memory->free);*/
	if(debug > 1000) {
		printk("--> petmem_init_process: Init complete!\n");
	}
	return map;
}

void petmem_deinit_process(struct mem_map * map) {

}

uintptr_t petmem_alloc_vspace(struct mem_map * map, u64 num_pages) {
	struct list_head *ptr;
	struct vaddr_reg *entry;
	struct vaddr_reg *new;
	uintptr_t address = 0;

	//	We allocate in 4KB chunks
	num_pages = num_pages * PAGE_SIZE_4KB;

	printk("Memory allocation for %lld pages\n", num_pages);

	//	Start searching for free pages at the beginning of the
	//	memory map. If we can't find any free space... well...
	list_for_each(ptr, &(map->memory->list)) {
		entry = list_entry(ptr, struct vaddr_reg, list);
		//	Check if the node is free
		if(debug > 100) {
			printk("--> Checking = Start: %lld, Length: %lld, Free: %d\n", entry->start, entry->length, entry->free);
		}
		if(entry->free) {
			//	We got lucky!
			if(entry->length == num_pages) {
				if(debug > 100) {
					printk("--> petmem_alloc_vspace: Found free entry of exact length!\n");
				}
				entry->free = 0;
				address = entry->start;
				break;
			}
			//	Check if we have enough free space...
			if(entry->length > num_pages) {
				if(debug > 100) {
					printk("--> petmem_alloc_vspace: Broke up free entry to handle request!\n");
				}
				//	We need to break up the free space
				new = kmalloc(sizeof(struct vaddr_reg), GFP_KERNEL);
				//	We can't really do anything if it can't be malloced... so don't bother checking...
				INIT_LIST_HEAD(&(new->list));
				new->start = entry->start + num_pages;
				new->length = entry->length - num_pages;
				new->free = 1;
				list_add(&(new->list), &(entry->list));

				entry->length = num_pages;
				entry->free = 0;

				if(debug > 100) {
					printk("--> petmem_alloc_vspace: New node stats = Start: %lld, Length: %lld, Free: %d\n", entry->start, entry->length, entry->free);
					printk("--> petmem_alloc_vspace: New free node stats = Start: %lld, Length: %lld, Free: %d\n", new->start, new->length, new->free);
				}
				address = entry->start;
				break;
			}
		}
	}
	printk("--> petmem_alloc_vspace: Returning address %lld\n", address);
	return address;
}

void petmem_dump_vspace(struct mem_map * map) {
	struct list_head *ptr;
	struct vaddr_reg *entry;
	printk("--> petmem_dump_vspace: Calling dump...\n");
	list_for_each(ptr, &(map->memory->list)) {
		entry = list_entry(ptr, struct vaddr_reg, list);
		printk("--> petmem_dump_vspace: Start - %lld, Length - %lld, Free - %d\n", entry->start, entry->length, entry->free);
	}
	return;
}

// Only the PML needs to stay, everything else can be freed
void petmem_free_vspace(struct mem_map * map, uintptr_t vaddr) {
	struct list_head *ptr;
	struct vaddr_reg *entry;
	struct vaddr_reg *next_entry;
	struct vaddr_reg *prev_entry;

	printk("Attempting to free address %lld\n", vaddr);

	list_for_each(ptr, &(map->memory->list)) {
		entry = list_entry(ptr, struct vaddr_reg, list);
		if(entry->start == vaddr) {
			//	We have found the address to free!
			entry->free = 1;
			next_entry = list_entry(ptr->next, struct vaddr_reg, list);
			if(next_entry->free) {
				entry->length = next_entry->length + entry->length;
				list_del(&(next_entry->list));
				kfree(next_entry);
			}
			prev_entry = list_entry(ptr->prev, struct vaddr_reg, list);
			if(prev_entry->free) {
				prev_entry->length = prev_entry->length + entry->length;
				list_del(&(entry->list));
				kfree(entry);
			}
		}
	}
	printk("--> petmem_free_space: Memory freed!");
	return;
}

/*
   error_code:
       1 == not present
       2 == permissions error
*/

int petmem_handle_pagefault(struct mem_map * map, uintptr_t fault_addr, u32 error_code) {

	//	We'll need this if we want to allocate pages later
	uintptr_t temp;
	pml4e64_t * cr3;
	pdpe64_t * pdp;
	pde64_t * pde;
	pte64_t * pte;
	u64 start, end, alloc_addr;
	int i;
	struct list_head *ptr;
	struct vaddr_reg *entry;
	struct vaddr_reg *next;

	printk("Fault Address %lld\n", fault_addr);
    printk("Fault addr 0x%lx\n", fault_addr);

	//	Find the virtual entry
	list_for_each(ptr, &(map->memory->list)) {
		entry = list_entry(ptr, struct vaddr_reg, list);
		next = list_entry(ptr->next, struct vaddr_reg, list);
		if(debug > 10) {
			printk("Comparing = Start: %lld, Next: %lld, Address: %lld\n", entry->start, next->start, fault_addr);
		}
		if(entry->start <= fault_addr && next->start > fault_addr) {
			//	Found the entry!
			break;
		}
	}
	start = entry->start;
	end = start + entry->length;

	if(debug > 10) {
		printk("--> petmem_handle_pagefault: Found node stats = Start: %lld, Length: %lld, Free: %d\n", entry->start, entry->length, entry->free);
		printk("Start: %lld, End: %lld\n", start, end);
	}

	for(alloc_addr = start; alloc_addr < end; alloc_addr++) {
		cr3 = (pml4e64_t *)__va( CR3_TO_PML4E64_PA( get_cr3() ) + PML4E64_INDEX( alloc_addr ) * 8 );
		if(!cr3->present) {
			printk("--> petmem_handle_pagefault: PML level not present; allocating...\n");
			temp = petmem_alloc_pages(1);
			for(i = 0; i < MAX_PML4E64_ENTRIES; i++) {
				memcpy(__va(temp) + (i * 8), cr3, 8);
			}
			cr3->present = 1;
			cr3->pdp_base_addr = PAGE_TO_BASE_ADDR( temp );
			printk("--> petmem_handle_pagefault: Added to cr3: %p\n", cr3->pdp_base_addr);
		}
        printk("Found pdp at location: %016lx\n",cr3->pdp_base_addr );
		pdp = __va( BASE_TO_PAGE_ADDR( cr3->pdp_base_addr ) + PDPE64_INDEX( alloc_addr ) * 8 );
		if(!pdp->present) {
			printk("--> petmem_handle_pagefault: PDP not present; allocating...\n");
			temp = petmem_alloc_pages(1);
			for(i = 0; i < MAX_PDPE64_ENTRIES; i++) {
				memcpy(__va(temp) + (i * 8), pdp, 8);
			}
			pdp->present = 1;
			pdp->pd_base_addr = PAGE_TO_BASE_ADDR( temp );
			printk("--> petmem_handle_pagefault: Added to pdp: %p\n", pdp->pd_base_addr);
		}
        printk("Found pd at location: %016lx\n",pdp->pd_base_addr );
		pde = __va( BASE_TO_PAGE_ADDR( pdp->pd_base_addr ) + PDE64_INDEX( alloc_addr )* 8 );
		if(!pde->present) {
			printk("--> petmem_handle_pagefault: PDE not present; allocating...\n");
			temp = petmem_alloc_pages(1);
			for(i = 0; i < MAX_PDE64_ENTRIES; i++) {
				memcpy(__va(temp) + (i * 8), pde, 8);
			}
			pde->present = 1;
			pde->pt_base_addr = PAGE_TO_BASE_ADDR( temp );
			printk("--> petmem_handle_pagefault: Added to pde: %p\n", pde->pt_base_addr);
		}

        printk("Found pte at location: %016lx\n",pde->pt_base_addr );
		pte = __va( BASE_TO_PAGE_ADDR( pde->pt_base_addr ) + PTE64_INDEX( alloc_addr ) * 8 );
		if(!pte->present) {
			printk("--> petmem_handle_pagefault: PTE not present; allocating...\n");
			temp = petmem_alloc_pages(1);
			for(i = 0; i < MAX_PTE64_ENTRIES; i++) {
				memcpy(__va(temp) + (i * 8), pte, 8);
			}
			pte->present = 1;
			pte->page_base_addr = PAGE_TO_BASE_ADDR( temp );
			printk("--> petmem_handle_pagefault: Added to pte: %p\n", pte->page_base_addr);
		}
	}
    return 0;
}
