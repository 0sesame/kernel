#include <stdint-gcc.h>
#include "VGA_driver.h"
#include "mystdio.h"
#include "ps2_driver.h"
#include "interrupt.h"
#include "serial.h"
#include "page_manager.h"
#include "mmu.h"

int kmain(void *args);
int kmain_virtual(void *args);

int kmain(void *args){
    void *kernel_stack;
    VGA_clear();
    IRQ_init();
    SER_init();
    GDT_init();
    PAGE_init(args);
    initialize_ps2_controller();
    initialize_ps2_keyboard();
    kernel_stack = MMU_init_virtual_mem(); 
    printk("moving stack to %p\n", kernel_stack);
    asm volatile("mov %0, %%rsp"
        ::"m" (kernel_stack));
    kmain_virtual(args);
    return 0;
}

void test_virtual_mem_alloc(void){
    void *page = (void*) 0x01;
    uint64_t *test_out;
    int count = 0;
    for(int i = 0; i < (2^14); i++){ // allocate about 65MiB of memory as test
        page = MMU_pf_alloc();
        if(!(page)){
            printk("memory test done\n");
            printk("%d\n", count);
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
        count++;
    }
    printk("Passed virtual memory test\n");
}

int kmain_virtual(void *args){
    //meme dream beam machine
    int on_stack = 1;
    //uint64_t *hm = (uint64_t *)0xFFFDEADBEEF;
    printk("on stack int %p\n", &on_stack);
    test_virtual_mem_alloc();
    while(1){
        asm volatile("hlt" : :);
    };
    return 0;
}
