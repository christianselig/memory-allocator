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
    real_buf2[0] = 's';
    real_buf2[1] = 'u';
    real_buf2[2] = 'c';
    real_buf2[3] = 'c';
    real_buf2[4] = 'e';
    real_buf2[5] = 's';
    real_buf2[6] = 's';
    real_buf2[7] = '\0';
    real_buf[0] = 't';
    real_buf[1] = 'e';
    real_buf[2] = 's';
    real_buf[3] = 't';
    real_buf[4] = '\0';
    while(i < 33000){

        buf = pet_malloc(10);
        buf[0] ='a';
        i++;
    }

    //Should output b then c
    printf("%s\n", real_buf);
    printf("%s\n", real_buf2);
    printf("Holy hell IT WORKED\n");
    return 0;
}
