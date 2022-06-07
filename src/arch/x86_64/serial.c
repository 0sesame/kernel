#include <stdbool.h>
#include "serial.h"
#include "irq_handlers.h"
#include "interrupt.h"
#include "mystdio.h"

#define PORT 0x3F8
#define LINE_CONTROL_REGISTER (PORT + 3)
#define MODEM_CONTROL_REGISTER (PORT + 4)
#define LINE_STATUS_REGISTER (PORT + 5)
#define INTERRUPT_ENABLE_REGISTER (PORT  + 1)
#define INTERRUPT_ID_REGISTER (PORT + 2)

#define TX_EMPTY_IRQ 4
#define TX_EMPTY_LSR (1 << 5)

#define PARITY_BIT 3
#define STOP_BIT 2
#define TX_INTERRUPT 1

static struct UartState uart_state;

void initialize_uart_state(struct UartState *state){
    // cosumer and producer at the same byte
    state->consumer = &state->buff[0];
    state->producer = &state->buff[0];
    state->end_of_buff = &state->buff[BUFF_SIZE - 1];
    state->busy = false;
}

void initialize_uart_hardware(void){
    uint8_t configuration = ((1 << 1) || 1) & ~(1 << PARITY_BIT) & ~(1 << STOP_BIT);
    outb(INTERRUPT_ENABLE_REGISTER, 0x00); // disable interrupts
    outb(LINE_CONTROL_REGISTER, configuration); // 8n1
    outb(INTERRUPT_ENABLE_REGISTER, (1 << TX_INTERRUPT)); // turn on tx empty interrupt
    outb(MODEM_CONTROL_REGISTER, (1 << 3));
}


bool uart_buffer_empty(struct UartState *uart_state){
    return uart_state->consumer == uart_state->producer;
}

char consume_byte(struct UartState *uart_state){
    char byte = *(uart_state->consumer);
    if (uart_state->consumer != uart_state->end_of_buff){
        uart_state->consumer++;
    }
    else{
        uart_state->consumer = &uart_state->buff[0];
    }

    return byte;
}

int transmit_empty(void){
    return inb(LINE_STATUS_REGISTER) & TX_EMPTY_LSR;
}

void start_tx(struct UartState *uart_state){
    char byte;
    if(uart_buffer_empty(uart_state)){ // buffer empty scenario
        return;
    }
    byte = consume_byte(uart_state);
    outb(PORT, byte);
}

void handle_line_interrupt(void){
    inb(LINE_STATUS_REGISTER);
}

int producer_add_byte(struct UartState *uart_state, char byte){
    if(uart_state->producer == uart_state->consumer - 1 ||
        (uart_state->producer == uart_state->end_of_buff && 
        uart_state->consumer == &(uart_state->buff[0]))){ // buffer is full
            return -1;
        }
    
    *(uart_state->producer) = byte;
    if (uart_state->producer != uart_state->end_of_buff){
        uart_state->producer++;
    }
    else{
        uart_state->producer = &uart_state->buff[0];
    }

    if(transmit_empty()){
        start_tx(uart_state);
    }
   return 0;
}

void SER_handle_interrupt(int interrupt_number, int error, void *args){
    CLI_IF;
    uint8_t interrupt_reason = inb(INTERRUPT_ID_REGISTER);
    if(interrupt_reason == (1 << TX_INTERRUPT)){
        uart_state.busy = false;
        start_tx(&uart_state);
    }
    else{
        handle_line_interrupt();
    }
    IRQ_end_of_interrupt(TX_EMPTY_IRQ);
    STI_IF;
}

void SER_init(void){
    CLI;
    initialize_uart_state(&uart_state);
    initialize_uart_hardware();
    IRQ_set_handler(TX_EMPTY_IRQ + PIC1_BASE, SER_handle_interrupt, (void *) 0);
    IRQ_set_clear_mask(TX_EMPTY_IRQ, 1); // clear bit 3 on pic to enable interrupts form pic
    STI;
}

int SER_write(char *byte, int len){
    // returns the number of bytes added to the serial buffer
    for(int i = 0; i < len; i++){
        if(producer_add_byte(&uart_state, byte[i]) < 0){
            return i;
        }
    }
    return len;
}