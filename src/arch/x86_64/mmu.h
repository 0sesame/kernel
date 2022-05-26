#ifndef MMU_H
#define MMU_H

#define ENTRIES_IN_PAGE_TABLE 512

struct PageTableIndex{
    uint64_t page_frame_offset:12;
    uint64_t p1_index:9;
    uint64_t p2_index:9;
    uint64_t p3_index:9;
    uint64_t p4_index:9;
    uint64_t sign_ext:16;
};
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
    uint64_t alloc_on_demand:3;
    uint64_t base_address:40; // address of page frame or entry in next table
    uint64_t avail2:11;
    uint64_t no_execute:1;
};

struct PageTable{
    struct PageTableEntry entries[ENTRIES_IN_PAGE_TABLE];
};


void *MMU_init_virtual_mem(void);
void *MMU_pf_alloc_many(int num);
void *MMU_pf_alloc(void);
void MMU_pf_free(void *pf);
void debug_page_table(struct PageTable *Pml4Table, uint64_t address);
struct PageTableEntry *traverse_page_tables_for_entry(struct PageTableEntry *entry, uint64_t *address, uint8_t init, uint64_t physical_addr, uint8_t table_level, uint8_t alloc_on_demand, uint8_t present, uint8_t cascade);

#endif