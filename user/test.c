#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "harness.h"


/* A simple test program.
 * You should modify this to perform additional checks
 */

int main(int argc, char ** argv) {
    init_petmem();

    char * buf = NULL;

    char * buf2 = NULL;


    buf = pet_malloc(8192);
	//buf = pet_malloc(100);

    printf("Allocated buf at %p\n", buf);

    pet_dump();


    buf[50] = 'H';
    buf[51] = 'e';
    buf[52] = 'l';
    buf[53] = 'l';
    buf[54] = 'o';
    buf[55] = ' ';
    buf[56] = 'W';
    buf[57] = 'o';
    buf[58] = 'r';
    buf[59] = 'l';
    buf[60] = 'd';
    buf[61] = '!';
    buf[62] = 0;

    buf[7100] = 'h';
    printf("%s\n", (char *)(buf + 50));

    pet_free(buf);
    buf = pet_malloc(8100);

    buf[50] = 'H';
    buf[51] = 'e';
    buf[52] = 'l';
    buf[53] = 'l';
    buf[54] = 'o';
    buf[55] = ' ';
    buf[56] = 'W';
    buf[57] = 'o';
    buf[58] = 'r';
    buf[59] = 'l';
    buf[60] = 'd';
    buf[61] = '!';
    buf[62] = 0;
   printf("%s\n", (char *)(buf + 50));

   pet_free(buf);

    return 0;
}
