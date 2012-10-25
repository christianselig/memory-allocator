
int init_petmem(void);


void * pet_malloc(size_t size);
void pet_free(void * addr);
void pet_dump();
void pet_invlpg(void * addr);
