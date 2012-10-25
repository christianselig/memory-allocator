/* Swap file implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>


#include "file_io.h"
#include "swap.h"

struct swap_space * swap_init(u32 num_pages) {
    return NULL;
}



void swap_free(struct swap_space * swap) {

}


int swap_out_page(struct swap_space * swap, u32 * index, void * page) {


    return -1;
}




int swap_in_page(struct swap_space * swap, u32 index, void * dst_page) {

    return -1;
}
