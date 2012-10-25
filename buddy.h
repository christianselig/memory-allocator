/* Copyright (c) 2007, Sandia National Laboratories */

#ifndef _LWK_BUDDY_H
#define _LWK_BUDDY_H

#include <linux/list.h>

/**
 * This structure stores the state of a buddy system memory allocator object.
 */
struct buddy_mempool {
	unsigned long    base_addr;    /** base address of the memory pool */
	unsigned long    pool_order;   /** size of memory pool = 2^pool_order */
	unsigned long    min_order;    /** minimum allocatable block size */

	unsigned long    num_blocks;   /** number of bits in tag_bits */
	unsigned long    *tag_bits;    /** one bit for each 2^min_order block
	                                *   0 = block is allocated
	                                *   1 = block is available
	                                */

        struct list_head node;

	struct list_head *avail;       /** one free list for each block size,
	                                * indexed by block order:
	                                *   avail[i] = free list of 2^i blocks
	                                */
};


/**
 * Each free block has one of these structures at its head. The link member
 * provides linkage for the mp->avail[order] free list, where order is the
 * size of the free block.
 */
struct block {
	struct list_head link;
	unsigned long    order;
};


extern struct buddy_mempool *
buddy_init(
	unsigned long base_addr,
	unsigned long pool_order,
	unsigned long min_order
);

extern void 
buddy_deinit(
	struct buddy_mempool * mp
);

extern void *
buddy_alloc(
	struct buddy_mempool *mp,
	unsigned long order
);

extern void
buddy_free(
	struct buddy_mempool *mp,
	const void *addr,
	unsigned long order
);


extern void
buddy_dump_mempool(
	struct buddy_mempool *mp
);

#endif
