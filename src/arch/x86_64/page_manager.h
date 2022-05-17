#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H
#include <stdint-gcc.h>

#define PAGE_SIZE 4096
#define MAX_MEMORY_REGIONS 10 // random number

struct UsableMemoryRegion{
    uint64_t start_address;
    uint64_t length;
};

struct PageHeader{
    void *next_page;
};

uint64_t PAGE_last_available_address(void);
void PAGE_init(void *);
void *PAGE_pf_alloc(void);
void PAGE_pf_free(void *page);
void PAGE_test(void);

#endif