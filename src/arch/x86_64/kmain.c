#include <stdint-gcc.h>
#include "VGA_driver.h"
#include "mystdio.h"
#include "ps2_driver.h"
#include "interrupt.h"
#include "serial.h"
#include "page_manager.h"
#include "mmu.h"
#include "malloc.h"

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
    for(int i = 0; i < 10000; i++){ // allocate about 40MiB of memory as test
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
    printk("Passed virtual memory test for %d pages\n", count);
}

void test_bad_mem_access(void){
    uint8_t *dont_write_here = (uint8_t *) ((uint64_t) 0x3 << 40);
    *dont_write_here = 4;
}

void test_kmalloc(int free){
    int *int_ptr;
    int size;
    int iterations = 100;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(sizeof(uint64_t));
        *int_ptr = i;
        if(*int_ptr != i){
            printk("Failed kmalloc test\n");
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 33;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 65;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 129;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 129;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 513;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 1025;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
    size = 2049;
    for(int i = 0; i < iterations; i++){
        int_ptr = kmalloc(size);
        for(int j = 0; j < (size / sizeof(int)); j++){
            *(int_ptr + j) = i;
        }
        for(int j = 0; j < (size / sizeof(int)); j++){
            if(*(int_ptr + j) != i){
                printk("Failed kmalloc test\n");
            };
        }
        printk("int_ptr on iteration %d\n", *int_ptr);
        if(free){
            kfree(int_ptr);
        }
    }
}

void stress_test_stack(int inc){
    uint64_t on_stack = inc;
    printk("on_stack: %ld\n", on_stack);
    printk("on_stack: %p\n", &on_stack);
    if(inc == 0){
        return;
    }
    stress_test_stack(inc - 1);
}

int kmain_virtual(void *args){
    //stress_test_stack(1000);
    test_virtual_mem_alloc();
    //test_bad_mem_access();
    initialize_heap();
    //test_kmalloc(0);
    test_kmalloc(1);
    print_pool_avail_blocks();
    printk("Memory tests passed\n");
    while(1){
        asm volatile("hlt" : :);
    };
    return 0;
}
