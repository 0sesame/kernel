#include <stdint-gcc.h>
#include "VGA_driver.h"
#include "mystdio.h"
#include "ps2_driver.h"
#include "interrupt.h"
#include "serial.h"
#include "page_manager.h"
#include "mmu.h"

int kmain(void *args);

int kmain(void *args){
    VGA_clear();
    IRQ_init();
    SER_init();
    GDT_init();
    PAGE_init(args);
    initialize_ps2_controller();
    initialize_ps2_keyboard();
    MMU_init_virtual_mem();
    while(1){
        asm volatile("hlt" : :);
    };
}
