#include "interrupt.h"
#include "malloc.h"
#include "proc.h"
#include "serial.h"

struct KbdState{
    struct UartState kbd_buff; // use uart state circ buff implementation
    struct ProcessQueue block_queue;
};

static struct KbdState *kbd;

struct KbdState *KBD_init(void){
    kbd = kmalloc(sizeof(struct KbdState));
    proc_queue_init(&(kbd->block_queue));
    initialize_uart_state(&(kbd->kbd_buff));
    return kbd;
}

void KBD_write(char byte){
    CLI_IF;
    producer_add_byte_to_buff(&(kbd->kbd_buff), byte);
    PROC_unblock_one(&(kbd->block_queue));
    STI_IF;
}

char KBD_read(void){
    CLI;
    while(uart_buffer_empty(&(kbd->kbd_buff))){
        PROC_block_on(&(kbd->block_queue), 1);
        CLI;
    }
    char res = consume_byte(&(kbd->kbd_buff));
    STI;
    return res;
};