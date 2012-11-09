#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "harness.h"


/* A simple test program.
 * You should modify this to perform additional checks
 */

int main(int argc, char ** argv) {
    init_petmem();
    int i = 0;
    char * buf = NULL;
    char * real_buf = NULL;
    char * real_buf2 = NULL;

    real_buf = pet_malloc(10);

    real_buf2 = pet_malloc(10);
    real_buf2[0] = 'c';
   real_buf[0] = 'b';
    while(i < 60000){

        buf = pet_malloc(10);
        buf[0] ='a';
        i++;
    }

    printf("%c\n", real_buf[0]);

    printf("%c\n", real_buf2[0]);
    return 0;
}
