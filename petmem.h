/* 
 * PetMem Control utility
 * (c) Jack Lange, 2012
 */

#ifndef __KERNEL__
char * dev_file = "/dev/petmem";
#endif


struct memory_range {
    unsigned long long base_addr;
    unsigned long long pages;
} __attribute__((packed));

struct alloc_request {
    // input
    unsigned long long size;

    // output 
    unsigned long long addr;    
} __attribute__((packed));


struct page_fault {
    unsigned long long fault_addr;
    unsigned int error_code;
} __attribute__((packed));


// IOCTLs
#define ADD_MEMORY     1
#define RELEASE_MEMORY 2

#define ALLOCATE       10
#define FREE           11

#define LAZY_ALLOC     30
#define LAZY_FREE      31
#define LAZY_DUMP_STATE 32

#define PAGE_FAULT     50
#define INVALIDATE_PAGE 51



#ifdef __KERNEL__
/* 128GB of virtual address space, that will hopefully be unused
   These are pulled from the user space memory region 
   Note that this could be a serious issue, because this range is not reserved 

   Based on my thorough investigation (running `cat /proc/self/maps` ~20 times), 
   this appears to sit between the heap and runtime libraries
*/
#define PETMEM_REGION_START 0x1000000000LL
#define PETMEM_REGION_END   0x3000000000LL



// Returns the current CR3 value
static inline uintptr_t get_cr3(void) {
    u64 cr3 = 0;

    __asm__ __volatile__ ("movq %%cr3, %0; "
			  : "=q"(cr3)
			  :
			  );

    return (uintptr_t)cr3;
}


static inline void invlpg(uintptr_t page_addr) {
    printk("Invalidating Address %p\n", (void *)page_addr);
    __asm__ __volatile__ ("invlpg (%0); "
			  : 
			  :"r"(page_addr)
			  : "memory"
			  );
}



#include <linux/types.h>

uintptr_t petmem_alloc_pages(u64 num_pages);
void petmem_free_pages(uintptr_t page_addr, u64 num_pages);

#endif
