#include "page_manager.h"
#include "string.h"
#include "mystdio.h"
#include "mmu.h"

#define END_OF_ID_MAP ((uint64_t) 1 << 40)

#define KERNEL_STACK_LAST_PAGE (((uint64_t) 0x2 << 40) - PAGE_SIZE)
#define KERNEL_STACK_PAGE_COUNT 100

#define KERNEL_STACKS_INDX 1
#define KERNEL_HEAP_START ((uint64_t) 0xF << 40)
#define USER_SPACE_INDX 16

static struct PageTable *Pml4Table;
static uint64_t virutal_kernel_heap_next_avail = KERNEL_HEAP_START;

struct PageTable* init_page_table_for_entry(struct PageTableEntry* entry, uint8_t alloc_on_demand, uint8_t present){
    struct PageTable *new_pt = (struct PageTable *) PAGE_pf_alloc();
    memset((void *) new_pt, 0, PAGE_SIZE); // 0 out pml4table
    if(new_pt == 0){
        printk("got null pointer from pf allocator\n");
    }
    entry->base_address = ((uint64_t) new_pt) >> 12; // address without 12 lsb's
    if(entry->base_address == 0){
        printk("there's bene a goof in the goof palace\n");
    }
    entry->present = present;
    entry->alloc_on_demand = alloc_on_demand;
    entry->writable = 1;
    entry->user_accesible = 0;
    return new_pt;

}

struct PageTableEntry *traverse_page_tables_for_entry(struct PageTableEntry *entry, uint64_t *address, uint8_t init, uint64_t physical_addr, uint8_t table_level, uint8_t alloc_on_demand, uint8_t present){
    // uses or initializes page tables at indices according to address, ending with putting physical_addr in the p1 table bas address
    struct PageTableIndex *page_table_index = (struct PageTableIndex *) address;
    uint16_t table_index;

    if(!(entry)){
        // if null entry is passed in, assumes a entry is needed in p4
        entry = &(Pml4Table->entries[page_table_index->p4_index]);
        table_level = 3;
    }

    if(table_level == 0){
        if(init){
            entry->base_address = physical_addr >> 12; // identity map address
            entry->present = present;
            entry->alloc_on_demand = alloc_on_demand;
            entry->writable = 1;
            entry->user_accesible = 0;
        }
        return entry;
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
            if(init){
                entry = &(init_page_table_for_entry(entry, alloc_on_demand, present)->entries[table_index]);
            }
            else{
                return entry; // if not initializing return null since no entry present to retrieve
            }
        }
        else{
            entry = &(((struct PageTable*) (((uint64_t) entry->base_address) << 12))->entries[table_index]);
        }
    }

    return traverse_page_tables_for_entry(entry, address, init, physical_addr, table_level - 1, alloc_on_demand, present);
}

struct PageTable *huge_table_p2_id_map(){
    struct PageTable *p4, *p3, *p2;
    uint64_t addr = 0x200000;
    p4 = PAGE_pf_alloc();
    p3 = PAGE_pf_alloc();
    p2 = PAGE_pf_alloc();
    p4->entries[0].base_address = ((uint64_t) p3) >> 12;
    p4->entries[0].present = 1;
    p4->entries[0].writable = 1;
    p3->entries[0].base_address = ((uint64_t) p2) >> 12;
    p3->entries[0].present = 1;
    p3->entries[0].writable = 1;
    for(int i = 0; i < ENTRIES_IN_PAGE_TABLE; i++){
        p2->entries[i].base_address = (addr * i) >> 12;
        p2->entries[i].huge_page = 1;
        p2->entries[i].present = 1;
        p2->entries[i].writable = 1;
    }
    return p4;
}

void debug_page_table(struct PageTable *Pml4Table, uint64_t address){
    struct PageTableIndex *page_indx = (struct PageTableIndex *) &address;
    struct PageTable *curr;
    printk("addr on stack: %lx\n", address);
    printk("addr translated: %lx\n", ((uint64_t) traverse_page_tables_for_entry(0, &address, 0, 0, 3, 0, 0)->base_address) << 12);
    printk("p4: %p\n", Pml4Table);
    curr = (struct PageTable *) ((uint64_t) Pml4Table->entries[page_indx->p4_index].base_address << 12);
    printk("p3: %p from p4 indx: %d\n", curr, page_indx->p4_index);
    curr = (struct PageTable *) ((uint64_t) curr->entries[page_indx->p3_index].base_address << 12);
    printk("p2: %p from p3 indx: %d\n", curr, page_indx->p3_index);
    curr = (struct PageTable *) ((uint64_t) curr->entries[page_indx->p2_index].base_address << 12);
    printk("p1: %p from p2 indx: %d\n", curr, page_indx->p2_index);
    curr = (struct PageTable *) ((uint64_t) curr->entries[page_indx->p1_index].base_address << 12);
    printk("page: %p from p1 indx: %d\n", curr, page_indx->p1_index);
}

void *MMU_pf_alloc(void){
    uint64_t temp;

    traverse_page_tables_for_entry(0, &virutal_kernel_heap_next_avail, 1, 0, 3, 1, 0); // alloc on demand, not present
    temp = virutal_kernel_heap_next_avail;
    virutal_kernel_heap_next_avail += PAGE_SIZE;

    return (void *)temp;
}

void MMU_pf_free(void *pf){
    uint64_t virtual_addr = (uint64_t) pf;

    // don't actually need to turn off present bit or alloc on demand stuff since we aren't gonna reuse this 
    uint64_t physical_addr = ((uint64_t) traverse_page_tables_for_entry(0, &virtual_addr, 0, 0, 3, 0, 0)->base_address) << 12; 
    PAGE_pf_free((void *) physical_addr);
}

void *MMU_init_virtual_mem(void){
    //returns pointer to start of kernel stack
    struct PageTableEntry cr3;
    memset((void *) &cr3, 0, sizeof(struct PageTableEntry)); // 0 out pml4table
    uint64_t *cr3_as_int;
    uint64_t page_addr;
    uint64_t last_physical_addr = PAGE_last_available_address();

    memset((void *) Pml4Table, 0, PAGE_SIZE); // 0 out pml4table
    for(page_addr = 0; page_addr < last_physical_addr; page_addr += PAGE_SIZE){
        traverse_page_tables_for_entry(0, &page_addr, 1, page_addr, 3, 0, 1);
    }

    cr3.base_address = ((uint64_t) Pml4Table >> 12);
    cr3_as_int = (uint64_t *) &cr3;

    page_addr = KERNEL_STACK_LAST_PAGE;
    for(int i = 0; i < KERNEL_STACK_PAGE_COUNT; i++){
        traverse_page_tables_for_entry(0, &page_addr, 1, (uint64_t) PAGE_pf_alloc(), 3, 0, 1);
        page_addr = page_addr - PAGE_SIZE;
    }

    asm volatile("mov %0, %%cr3" ::"r" (*cr3_as_int));

    return (void *) KERNEL_STACK_LAST_PAGE + PAGE_SIZE - 1;
}
