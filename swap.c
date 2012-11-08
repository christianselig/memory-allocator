/* Swap file implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>
#include <linux/string.h>

#include "file_io.h"
#include "swap.h"
#define POWER_4KB 12
#define BITS_IN_A_BYTE 3


struct swap_space * swap_init(u32 num_pages) {
    u32 pages, char_map_page_size;
    struct swap_space * swap = kmalloc(sizeof(struct swap_space), GFP_KERNEL);
    swap->swap_file = file_open("/tmp/cs2510.swap", O_RDWR);
    if(!(swap->swap_file)){
        //BIG PROBLEM!
        return (struct swap_space * ) 0x0;
    }
    swap->size = file_size(swap->swap_file);
    pages = swap->size >> POWER_4KB; // 4kb per page
    swap->size = pages;
    char_map_page_size = pages >> BITS_IN_A_BYTE;
    if(char_map_page_size % BITS_IN_A_BYTE != 0){
        char_map_page_size += 1;
    }

    swap->alloc_map = kmalloc(char_map_page_size, GFP_KERNEL);
    file_read(swap->swap_file, (swap->alloc_map), char_map_page_size, 0);

    return swap;
}


/*
 * Returns 0 if the space is free.
 * Returns 1 if the space is allocated.
 * Returns -1 if you tried to access reserved space or outside the bounds.
 */
int check_bitmap(struct swap_space * swap, u32 index){
    if(index < swap->size){
        u32 byte_location, bit_location;
        char b;
        byte_location = index >> 3;
        bit_location = index % 8;
        b = swap->alloc_map[byte_location];
        return (b & (1 << bit_location)) >> bit_location;
    }
    return -1;
}

void put_value(struct swap_space * swap, u32 index, u8 value){
    if(index < swap->size){
        u32 byte_location, bit_location;
        char b;
        byte_location = index >> 3;
        bit_location = index % 8;
        b = swap->alloc_map[byte_location];
        b &= ~(1 << bit_location);
        b |= value << bit_location;
        swap->alloc_map[byte_location] = b;
    }
}



void swap_free(struct swap_space * swap) {

}


int swap_out_page(struct swap_space * swap, u32 * index, void * page) {
    int i;
    for(i = 0; i < swap->size; i++){
        if(check_bitmap(swap, i) == 0){
            put_value(swap, i, 1);
            *index = i;
            file_write(swap->swap_file, page, 4096, i * 4096);
            printk("FOUND DAT FILE AT %d\n", i);
            break;
        }
    }
    return -1;
}




int swap_in_page(struct swap_space * swap, u32 index, void * dst_page) {
    file_read(swap->swap_file, dst_page, 4096, index * 4096);
    put_value(swap, index, 0);
    return -1;
}
