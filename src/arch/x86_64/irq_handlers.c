#include "irq_handlers.h"
#include "interrupt.h"
#include "ps2_driver.h"
#include "mystdio.h"
#include "mmu.h"
#include "page_manager.h"

void IRQ_handle_keyboard(int interrupt_number, int error, void *args){
    char key = get_key_press();
    if(key){
        printk("%c", key);
    }
    // IRQ_end_of_interrupt wants associated PIC line of interrupt, so sub base
    IRQ_end_of_interrupt(interrupt_number - PIC1_BASE); 
}

void IRQ_handle_timeout(int interrupt_number, int error, void *args){
    printk("Receivied timeout(?) interrupt: %x\n", interrupt_number);
    // IRQ_end_of_interrupt wants associated PIC line of interrupt, so sub base
    IRQ_end_of_interrupt(interrupt_number - PIC1_BASE); 
}

void IRQ_handle_div0(int interrupt_number, int error, void *args){
    ((int *) 0xDEADBEEF)[0] = 12;
}

void IRQ_page_fault(int interrupt_number, int error, void *args){
    uint64_t addr = 0;
    struct PageTableEntry *pt_entry;
    void *new_page;

    asm volatile("mov %%cr2, %0" :"=r" (addr));

    pt_entry = traverse_page_tables_for_entry(0, &addr, 0, 0, 3, 0, 0);

    if(pt_entry->alloc_on_demand){
        new_page = PAGE_pf_alloc();
        if(!(new_page)){
            printk("System failed on-demand allocation for %p, out of physical mem\n", (void *)addr);
            asm volatile("hlt" : :);
        }
        traverse_page_tables_for_entry(0, &addr, 1, (uint64_t) new_page, 3, 0, 1); // init and set present
    }
    else{
        printk("Page fault error: %d\n", error);
        printk("Page fault addr: %p\n", (void *) addr);
        printk("Using the only page table configured thus far\n");
        asm volatile("hlt" : :);
    }

}