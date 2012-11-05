
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include  "../petmem.h"
#include "harness.h"



static struct sigaction dfl_segv_action;
static int fd = 0;



void * pet_malloc(size_t size) {
    struct alloc_request req;
    memset(&req, 0, sizeof(struct alloc_request));

    req.size = size;
    
    ioctl(fd, LAZY_ALLOC, &req); 

    return (void *)req.addr;
}

void pet_free(void * addr) {
    ioctl(fd, LAZY_FREE, addr);
}


void pet_dump() {
    ioctl(fd, LAZY_DUMP_STATE, NULL);

    return;
}

void pet_invlpg(void * addr) {
    ioctl(fd, INVALIDATE_PAGE, addr);
    return;
}


static void segv_handler(int signum, siginfo_t * info, void * context) {
    struct page_fault fault;

    if (signum != SIGSEGV) {
	printf("Well that is strange...\n");
	return;
    }  

    //    printf("SIGSEGV\n");

    // si_addr -> fault addr

    // si_code == 1 -> not present
    // si_code == 2 -> permission error


    fault.fault_addr = (unsigned long long)info->si_addr;
    fault.error_code = info->si_code;

    if (ioctl(fd, PAGE_FAULT, &fault)) {
	struct sigaction old_action;
	// if ioctl returns 1, then handler failed
	// we need to turn off our handler, and resignal to crash

	sigaction(SIGSEGV, &dfl_segv_action, &old_action);
	kill(getpid(), SIGSEGV);
    }

}


int init_petmem(void) {
    struct sigaction action;

    action.sa_sigaction = segv_handler;

    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask,SIGSEGV);

    action.sa_flags = SA_RESTART | SA_SIGINFO;
    
    sigaction(SIGSEGV, &action, &dfl_segv_action);


    fd = open(dev_file, O_RDONLY);

    
    if (fd == -1) {
	fprintf(stderr, "Error opening petmem device file (%s)\n", dev_file);
	return -1;
    }

    return 0;
}



