#include "page_manager.h"
#include "string.h"
#include "mystdio.h"

#define ENTRIES_IN_PAGE_TABLE 512
#define END_OF_ID_MAP (uint64_t) 1 << 40
#define KERNEL_STACKS_INDX 1
#define KERNEL_HEAP_INDX 15
#define USER_SPACE_INDX 16


struct PageTableEntry{
    uint64_t present:1; // is in memory
    uint64_t writable:1; // can write
    uint64_t user_accesible:1; // users can access, otherwise only kernel
    uint64_t write_through_cache:1; // write through the cache to memory
    uint64_t disable_cache:1; // no cache for this page
    uint64_t accessed:1; // set when page is used
    uint64_t dirty:1; // set when write to page occurs
    uint64_t huge_page:1; // 0 for p1 and p4, 1gig in p3, 2mb in p2
    uint64_t global:1; // keep page in cache on address space switch
    uint64_t avail_alloc_on_demand:3;
    uint64_t base_address:40; // address of page frame or entry in next table
    uint64_t avail2:11;
    uint64_t no_execute:1;
};

struct PageTable{
    struct PageTableEntry entries[ENTRIES_IN_PAGE_TABLE];
};

struct PageTableIndex{
    uint64_t page_frame_offset:12;
    uint64_t p1_index:9;
    uint64_t p2_index:9;
    uint64_t p3_index:9;
    uint64_t p4_index:9;
    uint64_t sign_ext:16;
};


struct PageTable *Pml4Table;

struct PageTable* init_page_table_for_entry(struct PageTableEntry* entry){
    struct PageTable *new_pt = (struct PageTable *) PAGE_pf_alloc();
    memset((void *) new_pt, 0, PAGE_SIZE); // 0 out pml4table
    entry->base_address = ((uint64_t) new_pt) >> 12; // address without 12 lsb's
    entry->present = 1;
    entry->writable = 1;
    entry->user_accesible = 0;
    return new_pt;

}

void init_page_tables_for_address(struct PageTableEntry *entry, uint64_t *address, uint64_t physical_addr, uint8_t table_level){
    // uses or initializes page tables at indices according to address, ending with putting physical_addr in the p1 table bas address

    struct PageTableIndex *page_table_index = (struct PageTableIndex *) address;
    uint16_t table_index;

    if(!(entry)){
        // if null entry is passed in, assumes a entry is needed in p4
        entry = &(Pml4Table->entries[page_table_index->p4_index]);
        table_level = 3;
    }

    if(table_level == 0){
        entry->base_address = physical_addr >> 12; // identity map address
        entry->present = 1;
        entry->writable = 1;
        entry->user_accesible = 0;
        return;
    }
 
    else{
        if(table_level == 3){
            table_index = page_table_index->p3_index;
        }
        else if(table_level == 2){
            table_index = page_table_index->p2_index;
        }
        else{
            table_index = page_table_index->p1_index;
        }

        if(!(entry->present)){
            entry = &(init_page_table_for_entry(entry)->entries[table_index]);
        }
        else{
            entry = &(((struct PageTable*) (((uint64_t) entry->base_address) << 12))->entries[table_index]);
        }
    }

    init_page_tables_for_address(entry, address, physical_addr, table_level - 1);
}

void MMU_init_virtual_mem(void){
    Pml4Table = (struct PageTable *) PAGE_pf_alloc();
    uint64_t last_physical_addr = PAGE_last_available_address();
    memset((void *) Pml4Table, 0, PAGE_SIZE); // 0 out pml4table
    for(uint64_t page_addr = 0; page_addr < last_physical_addr; page_addr += PAGE_SIZE){
        init_page_tables_for_address(0, &page_addr, page_addr, 3);
    }
    asm volatile("mov %%cr3, %0"
        ::"r" (Pml4Table));
}
