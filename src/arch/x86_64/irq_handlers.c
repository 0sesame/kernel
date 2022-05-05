#include "irq_handlers.h"
#include "interrupt.h"
#include "ps2_driver.h"
#include "mystdio.h"

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