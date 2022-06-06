#include <stddef.h>
#include <stdint-gcc.h>
#include "mystdio.h"
#include "mmu.h"
#include "page_manager.h"
#include "mystdio.h"

#define POOL_INIT_PAGES 64 // init about 2.5Mb on the heap for each pool

struct FreeList{
    struct FreeList *next;
};

struct HeapPool{
    int max_size;
    int available_blocks;
    struct FreeList *head;
};

struct HeapPoolBlockHeader{
    struct HeapPool *pool;
    size_t size;
};

enum pool_sizes{SIZE_32, SIZE_64, SIZE_128, SIZE_512, SIZE_1024, SIZE_2048};
static int pool_sizes_bytes[6] = {32, 64, 128, 512, 1024, 2048};
static struct HeapPool heap_pools[6];

void initialize_free_list(struct FreeList *head, int blocks, uint64_t block_offset){
    // put pointer to next block in first 8 bytes of free block
    struct FreeList *curr = head;
    for(int i = 1; i < blocks; i++){
        curr->next = (struct FreeList *) (((uint8_t *) curr) + block_offset); //add block_offset # of bytes to curr ptr to get next block
        curr = curr->next;
    }
    curr->next = 0x0;
    return;
}

void initialize_heap_pool(struct HeapPool *heap_pool, enum pool_sizes size, int pages_to_init){
    int size_in_bytes = pool_sizes_bytes[size];
    heap_pool->max_size = size_in_bytes;
    heap_pool->head = MMU_pf_alloc_many(pages_to_init);
    heap_pool->available_blocks = (PAGE_SIZE / size_in_bytes ) * pages_to_init;
    initialize_free_list(heap_pool->head, heap_pool->available_blocks, size_in_bytes);
}

void initialize_heap(void){
    printk("initializing heap\n");
    for(int i = SIZE_32; i <= SIZE_2048; i++){
        initialize_heap_pool(&(heap_pools[i]), i, POOL_INIT_PAGES);
        printk("heap pool avail %d\n", heap_pools[i].available_blocks);
    }
}

void *allocate_block_from_pool(struct HeapPool *pool, enum pool_sizes size){
    int size_bytes = pool_sizes_bytes[size];
    struct HeapPoolBlockHeader *block = (struct HeapPoolBlockHeader *) pool->head;
    if(!(pool->available_blocks)){
        initialize_heap_pool(pool, size, 1); // init a new page for pool
    }
    pool->head = pool->head->next;
    pool->available_blocks -= 1;
    block->pool = pool; 
    block->size = size_bytes;

    return (void *) (block + 1); // return address after block header
}

void *allocate_too_large_for_pool(size_t size){
    size = (size + sizeof(struct HeapPoolBlockHeader));
    int pages_needed = size / PAGE_SIZE + 1;
    struct HeapPoolBlockHeader *block = MMU_pf_alloc_many(pages_needed);
    block->pool = 0x0;
    block->size = size;

    return (void *) (block + 1); // return address after block header
}

void *kmalloc(size_t size){
    size = size + sizeof(struct HeapPoolBlockHeader); //avail size of each block is smaller than blcok size
    for(int i = SIZE_32; i <= SIZE_2048; i++){
        if(size <= pool_sizes_bytes[i]){
            return allocate_block_from_pool(&(heap_pools[i]), i);
        }
    }
    return allocate_too_large_for_pool(size);
}

void *kcalloc(size_t size){ // returns 0'd out memory at void *
    void *ret = kmalloc(size);
    memset(ret, 0, size);
    return ret;
}

void free_block_from_pool(struct HeapPoolBlockHeader *header){
    printk("Free blocking from pool %d with pool %p\n", header->pool->max_size, header->pool);
    struct FreeList *temp = header->pool->head;
    header->pool->head = (struct FreeList *) header;
    header->pool->head->next = temp;
    header->pool->available_blocks += 1;
    printk("available blocks %d\n", header->pool->available_blocks);
}

void print_pool_avail_blocks(void){
    for(int i = SIZE_32; i <= SIZE_2048; i++){
        printk("available blocks in pool %d: %d\n", pool_sizes_bytes[i], heap_pools[i].available_blocks);
    }
}

void kfree(void *to_free){
    int allocd_pages;
    struct HeapPoolBlockHeader *header = (struct HeapPoolBlockHeader *) to_free;
    header = header - 1; // point at actual header
    if(header->pool){ // if pointing at actual pool, not free from too large block
        free_block_from_pool(header);
    }
    else{
        allocd_pages = (header->size / PAGE_SIZE) + 1;
        for(int i = 0; i < allocd_pages; i++){
            MMU_pf_free(((uint8_t *) header) + i * PAGE_SIZE);
        }
    }
}