/* Swap file implementation
 * (c) Jack Lange, 2012
 */

#include <linux/slab.h>
#include <linux/string.h>

#include "file_io.h"
#include "swap.h"
#define POWER_4KB 12
#define BITS_IN_A_BYTE 8

struct swap_space * swap_init(u32 num_pages) {
    u32 pages, char_map_page_size;
    int i;
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
    swap->reserved_blocks = (char_map_page_size >> POWER_4KB);

    // Gotta get that last little block if it is there!
    if(char_map_page_size % POWER_4KB != 0){
        swap->reserved_blocks += 1;
    }
    swap->alloc_map = kmalloc(char_map_page_size, GFP_KERNEL);
    i = 0;
    while(i < char_map_page_size){
        swap->alloc_map[i] = 0;
        i++;
    }
    return swap;
}


/*
 * Returns 0 if the space is free.
 * Returns 1 if the space is allocated.
 * Returns -1 if you tried to access reserved space or outside the bounds.
 */
int check_bitmap(struct swap_space * swap, u32 index){
    if(index < swap->size && index > swap->reserved_blocks){
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
    if(index < swap->size && index >= swap->reserved_blocks){
         u32 byte_location, bit_location;
        char b;
        byte_location = index >> 3;
        bit_location = index % 8;
        b = swap->alloc_map[byte_location];
        b &= ~(1 << bit_location);
        b |= value << bit_location;
        swap->alloc_map[byte_location] = b;
        //file_write(swap->swap_file, &b, 1, byte_location);
    }
}



void swap_free(struct swap_space * swap) {

}


int swap_out_page(struct swap_space * swap, u32 * index, void * page) {
    char * charz = swap->alloc_map;
    //int i;
    /*
    for(i = swap->reserved_blocks + 1; i < swap->size; i++){
        if(check_bitmap(swap, i) == 0){
            put_value(swap, i, 1);
            printk("I PUT IN %d", check_bitmap(swap, i));
        }
    }
    */
    printk("Amount of pages %d\n", swap->reserved_blocks);
    printk("Can hold %lld pages and 0 %d\n", swap->size, *(charz+2));
    return -1;
}




int swap_in_page(struct swap_space * swap, u32 index, void * dst_page) {

    return -1;
}
