#include <stddef.h>

void initialize_heap(void);
void print_pool_avail_blocks(void);
void *kmalloc(size_t size);
void kfree(void *to_free);