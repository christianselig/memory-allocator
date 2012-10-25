/* Swap file implementation
 * (c) Jack Lange, 2012
 */

#include <linux/fs.h>

#ifndef __SWAP_H__
#define __SWAP_H__

struct swap_space {
    struct file * swap_file;

    /* add your own fields here */
};


struct swap_space * swap_init(u32 num_pages);
void swap_free(struct swap_space * swap);


int swap_out_page(struct swap_space * swap, u32 * index, void * page);
int swap_in_page(struct swap_space * swap, u32 index, void * dst_page);

#endif
