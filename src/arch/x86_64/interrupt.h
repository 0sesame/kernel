#include <stdint-gcc.h>

#define INTERRUPT_COUNT 256
#define PIC1_BASE 0x20
#define PIC2_BASE 0x28
#define CLI cli()
#define STI sti()

#define KERNEL_SEGMENT_OFFSET 0x08

// CLI_IF and STI_IF should always be paired
#define CLI_IF uint8_t enable_ints = 0; if(are_interrupts_enabled()){enable_ints = 1; CLI;}
#define STI_IF if(enable_ints){STI;}

struct IdtEntryOptions{
    uint16_t ist:3; // entry in interrupt stack table, defines which stack to use
    uint16_t reserved:5; // don't use
    uint16_t is_trap:1; // if 0 then interrupt, if 1 then trap
    uint16_t ones_padding:3; // all 1's
    uint16_t zero_padding:1; // all 0's
    uint16_t privilege_required:2; // DPL like in GDT, should always be 0 for us
    uint16_t present:1; // should be 1 if this entry is configured
}__attribute__((packed));

struct IdtEntry{
    uint16_t f_pointer_first_16; // first 16 bits of interrupt handler func
    uint16_t gdt_selector;
    struct IdtEntryOptions options; // options defined in bitfield
    uint16_t f_pointer_second_16; // second 16 bits of interrupt handler func
    uint32_t f_pointer_last_32; // last 32
    uint32_t reserved;
}__attribute__((packed));

struct Idt{
    struct IdtEntry entries[INTERRUPT_COUNT];
}__attribute__((packed));

struct TaskStateSegment{
    uint32_t reserved1;
    uint64_t privilege_stack_table[3];
    uint64_t reserved2;
    uint64_t interrupt_stack_table[7];
    uint64_t reserved3;
    uint16_t reserved4;
    uint16_t iomap_base;
}__attribute__((packed));

struct TssDescriptor{
    uint16_t segment_limit;
    uint16_t base_address_low16;
    uint8_t base_address_second8;
    uint8_t type:4;
    uint8_t zero:1;
    uint8_t dpl:2;
    uint8_t present:1;
    uint8_t segment_limit_last4:4;
    uint8_t avl:1;
    uint8_t reserved:2;
    uint8_t g:1;
    uint8_t base_address_third8;
    uint32_t base_address_high32;
    uint32_t zeros_reserved;
}__attribute__((packed));

struct Gdt{
    uint64_t null_descriptor;
    uint64_t kernel_code_descriptor;
    struct TssDescriptor tss_desc;
}__attribute__((packed));

typedef void (*irq_handler_t)(int, int, void *);

int are_interrupts_enabled(void);
void sti(void);
void cli(void);
void IRQ_end_of_interrupt(int irq);
void IRQ_set_clear_mask(unsigned int pic_irq_line, int clear);
void IRQ_set_handler(int irq, irq_handler_t handler, void *arg);
void IRQ_init(void);
void GDT_init(void);
void IDT_init(struct Idt *);