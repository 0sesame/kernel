#include <stdint-gcc.h>
#include <stdbool.h>
#include "mystdio.h"
#include "multiboot.h"
#include "page_manager.h"

#define PAGE_SIZE 4096
#define INIT_PAGE_COUNT 10000
#define PAGE_TEST_COUNT 20

static struct UsableMemoryRegion usable_memory_regions[MAX_MEMORY_REGIONS + 1]; // terminated with all 0 struct
void *never_allocated_head;
static struct PageHeader *free_list_head = (struct PageHeader *) 0x0;
uint64_t last_available_addr = 0;
static void *pages[PAGE_TEST_COUNT];

bool is_in_usable_memory(uint64_t start_address, uint64_t end_address){
    int i = 0;
    uint64_t end_of_usable_region;
    while(!(usable_memory_regions[i].length == 0 && usable_memory_regions[i].start_address == 0)){
        end_of_usable_region = usable_memory_regions[i].start_address + usable_memory_regions[i].length;
        if(start_address >= usable_memory_regions[i].start_address && end_address < end_of_usable_region){
            return true;
        }
        i++;
    }
    return false;
}

bool is_usable_page_start(void *start){
    return is_in_usable_memory((uint64_t) start, ((uint64_t) start) + PAGE_SIZE) && 
            !(is_in_use_by_kernel((uint64_t) start, ((uint64_t) start) + PAGE_SIZE));
}

void *get_next_available_never_allocd_page(uint64_t init_page_start){
    /* starting at init_page_start, searches through memory for a usable never allocated page */
    int i = 0;
    uint64_t end_of_usable_region;
    while(!(usable_memory_regions[i].length == 0 && usable_memory_regions[i].start_address == 0)){
        end_of_usable_region = usable_memory_regions[i].start_address + usable_memory_regions[i].length;
        if(init_page_start + PAGE_SIZE >= end_of_usable_region){
            if(init_page_start >= last_available_addr){
                break;
            }
            i++;
            continue;
        }

        if(init_page_start < usable_memory_regions[i].start_address){
            init_page_start = usable_memory_regions[i].start_address;
        }

        if(init_page_start >= usable_memory_regions[i].start_address && init_page_start < end_of_usable_region){
            while(!(is_usable_page_start((void *) init_page_start)) && init_page_start < last_available_addr && init_page_start < end_of_usable_region){ 
                init_page_start = init_page_start + PAGE_SIZE;
            }
            if(init_page_start >= end_of_usable_region){
                i++;
                continue;
            }
            return (void *) init_page_start;
        }
        i++;
    }
    return (void *) last_available_addr;
}

void *PAGE_pf_alloc(void){
    void *temp;
    if(free_list_head){
        temp = (void *) free_list_head;
        free_list_head = free_list_head->next_page;
        return temp;
    }
    if((uint64_t) never_allocated_head >= last_available_addr){
        return 0x0;
    }
    temp = never_allocated_head;
    never_allocated_head = (void *) get_next_available_never_allocd_page((uint64_t) never_allocated_head + PAGE_SIZE);
    return temp;
}

void PAGE_pf_free(void *page){
    struct PageHeader *page_header = (struct PageHeader *) page;
    page_header->next_page = (void *) free_list_head;
    free_list_head = page_header;
}

void PAGE_init(void *multiboot_tags){
    struct MultibootHeader *header = (struct MultibootHeader *) multiboot_tags;
    int usable_memory_regions_count;
    int i;
    uint64_t end_of_usable_region;
    printk("multiboot tag pointer: %p\n", multiboot_tags);
    printk("total tag size: %u\n", header->total_tag_size);
    parse_multiboot_tags(usable_memory_regions, (void *) (header + 1), header->total_tag_size, &usable_memory_regions_count);

    while(!(usable_memory_regions[i].length == 0 && usable_memory_regions[i].start_address == 0)){
        end_of_usable_region = usable_memory_regions[i].start_address + usable_memory_regions[i].length;
        if(end_of_usable_region > last_available_addr){
            last_available_addr = end_of_usable_region;
        }
        i++;
    }

    never_allocated_head = (void *) get_next_available_never_allocd_page(PAGE_SIZE);
}

void PAGE_test(void){
    void *page;
    uint64_t *test_out;
    for(int i = 0; i < PAGE_TEST_COUNT; i++){
        pages[i] = PAGE_pf_alloc();
        printk("Got page %p\n", pages[i]);
    } 
    for(int i = 0; i < PAGE_TEST_COUNT; i++){
        PAGE_pf_free(pages[i]);
        printk("Freed page %p\n", pages[i]);
    } 
    while(page){
        page = PAGE_pf_alloc();
        if(!(page)){
            return;
        }
        for(int j=0; j < PAGE_SIZE / (sizeof(uint64_t)); j++){
            ((uint64_t *) page)[j] = (uint64_t) page;
        }
        test_out = (uint64_t *) page;
        for(int j=0; j < PAGE_SIZE / (sizeof(uint64_t)); j++){
            if(*test_out != (uint64_t) page){
                printk("got %lx in page %p, not right\n", *test_out, page);
            }
            test_out++;
        }
    }
}