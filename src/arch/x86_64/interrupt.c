#include "interrupt.h"
#include "mystdio.h"
#include "isr_asm_labels.h"
#include "irq_handlers.h"
#include "syscall.h"

#define FIRST_16_64_BIT 0xFFFF

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01 // configuration command word
#define ICW4_8086 0x01 // why 8086 mode? just trusting osdev wiki
#define ICW4_AUTO 0x02

#define PIC1_DATA 0x21
#define PIC1_COMMAND 0x20
#define PIC2_DATA 0xA1
#define PIC2_COMMAND 0xA0
#define PIC_EOI 0x20
#define PIC_READ_IRR 0x0a
#define PIC_READ_IRS 0x0b

#define KERNEL_SEGMENT_OFFSET 0x08

#define DF 8
#define PF 14
#define GP 13

static struct {
    void *arg;
    irq_handler_t handle;
}irq_table[INTERRUPT_COUNT];

static struct Idt global_idt;
static struct TaskStateSegment tss;
static struct Gdt gdt;
static uint64_t df_interrupt_stack[256];
static uint64_t pf_interrupt_stack[256];
static uint64_t gp_interrupt_stack[256];
int interrupts_enabled = 0;

void PIC_remap(int offset1, int offset2){
    // remaps interrupts because of conflict with cpu exceptions
    unsigned char holder1, holder2;

    holder1 = inb(PIC1_DATA); // masks are in data registers
    holder2 = inb(PIC2_DATA); // masks are in data registers

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();
    outb(PIC1_DATA, 4); // pic 1 needs to know it has pic at irq2
    io_wait();
    outb(PIC2_DATA, 2); // pic 2 needs to know its cascade id is 2
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, holder1);
    outb(PIC2_DATA, holder2);
}

void sti(void){
    asm volatile("sti" ::);
    interrupts_enabled = 1;
}

void cli(void){
    asm volatile("cli" ::);
    interrupts_enabled = 0;
}

int are_interrupts_enabled(void){
    return interrupts_enabled;
}

void unhandled_interrupt(int interrupt_number, int err, void * args){
    printk("unhandled interrupt: %d, address: %p\n", interrupt_number, &interrupt_number);
    asm volatile("hlt" ::);
}

void handle_irq(int irq_number, int error, void *args){
    if(irq_number < 0 || irq_number >= INTERRUPT_COUNT){
        printk("Invalid interrupt number %d, can't handle interrupt\n", irq_number);
        return;
    }
    if(!(irq_table[irq_number].handle)){
        printk("C interrupt handler pointer is NULL, can't handle interrupt\n");
        return;
    }
    irq_table[irq_number].handle(irq_number, error, args);
}

int IRQ_get_mask(int pic_irq_line){
    uint16_t port;
    if(pic_irq_line >= 8){
        port = PIC2_DATA;
        pic_irq_line -= 8;
    }
    else{
        port = PIC1_DATA;
    }
    return (inb(port) & (1 << pic_irq_line)) >> pic_irq_line;
}

void IRQ_set_clear_mask(unsigned int pic_irq_line, int clear){
    uint16_t port;
    uint8_t val;

    if(pic_irq_line >= 8){ // if pic2 then use pic2 data port and offset line
        port = PIC2_DATA;
        pic_irq_line -= 8;
    }
    else{
        port = PIC1_DATA;
    }

    // set mask to current mask but with pic_irq_line bit set
    if(clear){
        val = inb(port) & ~(1 << pic_irq_line);
    }
    else{
        val = inb(port) | (1 << pic_irq_line);
    }
    outb(port, val);
}

void IRQ_set_handler(int irq, irq_handler_t handler, void *arg){
    irq_table[irq].handle = handler;
    irq_table[irq].arg = arg;
}

void IRQ_end_of_interrupt(int irq){
    if(irq >= 8){
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void TSS_setup_descriptor(void){
    uint64_t tss_addr = (uint64_t) &tss;
    gdt.tss_desc.segment_limit = sizeof(struct TaskStateSegment);
    gdt.tss_desc.segment_limit_last4 = 0;
    gdt.tss_desc.base_address_low16 = tss_addr & 0xFFFF;
    gdt.tss_desc.base_address_second8 = (tss_addr >> 16) & 0xFF;
    gdt.tss_desc.base_address_third8 = (tss_addr >> 24) & 0xFF;
    gdt.tss_desc.base_address_high32 = (tss_addr >> 32) & 0xFFFFFFFF;
    gdt.tss_desc.type = 0x09; // sets available tss
    gdt.tss_desc.avl = 0;
    gdt.tss_desc.g = 0;
    gdt.tss_desc.present = 1;
    gdt.tss_desc.zero = 0;
    gdt.tss_desc.zeros_reserved = 0;
    gdt.tss_desc.dpl = 0; // privilege
};

void TSS_init(void){
    tss.reserved1 = 0;
    tss.reserved2 = 0;
    tss.reserved3 = 0;
    tss.reserved4 = 0;
    tss.interrupt_stack_table[0] = (uint64_t) df_interrupt_stack + 255;
    tss.interrupt_stack_table[1] = (uint64_t) pf_interrupt_stack + 255;
    tss.interrupt_stack_table[2] = (uint64_t) gp_interrupt_stack + 255;
};

void GDT_init(void){
    uint64_t one = 1;
    gdt.null_descriptor = 0;
    gdt.kernel_code_descriptor = (one << 43) | (one << 44) | (one << 47) | (one << 53);
    TSS_setup_descriptor();

    static struct{
        uint16_t table_limit;
        void * table_address;
    }__attribute__((packed)) table_pointer; 
    table_pointer.table_limit = (sizeof(gdt));
    table_pointer.table_address = (void *) &gdt;
    uint16_t offset = (sizeof(struct Gdt) - sizeof(struct TssDescriptor));
    asm volatile ("lgdt %0" ::"m" (table_pointer));
    asm volatile("ltr %0"::"m" (offset));////int constant
    TSS_init();
}

void IRQ_init(void){
    // initialize all global interrupt table and PIC, register IRQ handlers
    // sets up task segment for critical faults and RELOADS GDT
    CLI;

    PIC_remap(PIC1_BASE, PIC2_BASE); // changes how interrupts are communicated
    IDT_init(&global_idt);
    // currently init all pointers to "null" to check output
    for(int i = 0; i < INTERRUPT_COUNT; i++){
        irq_table[i].handle = unhandled_interrupt;
    }
    for(int i =0; i <= 0xF; i++){
        IRQ_set_clear_mask(i, 0); // set mask for all interrupts
    }
    IRQ_set_clear_mask(1, 1); // clear mask for keyboard interrupt

    IRQ_set_handler(0, IRQ_handle_div0, (void *) 0);
    IRQ_set_handler(14, IRQ_page_fault, (void *) 0);
    IRQ_set_handler(YIELD_INT_NUM, IRQ_yield, (void *) 0);
    IRQ_set_handler(PIC1_BASE, IRQ_handle_timeout, (void *) 0);
    IRQ_set_handler(1 + PIC1_BASE, IRQ_handle_keyboard, (void *) 0);
    STI;
}

void initialize_options(struct IdtEntry *entry, int is_trap){
    entry->options.ist = 0;
    entry->options.reserved = 0;
    entry->options.is_trap = is_trap;
    entry->options.ones_padding = 7;
    entry->options.zero_padding = 0;
    entry->options.privilege_required = 0;
    entry->options.present = 1;
}

void IDT_entry_function_pointer_init(struct IdtEntry *entry, void (*func)()){
    entry->f_pointer_first_16 = (uint16_t) (unsigned long) (func) & FIRST_16_64_BIT;
    entry->f_pointer_second_16 = (uint16_t) ((unsigned long) (func) >> 16) & FIRST_16_64_BIT;
    entry->f_pointer_last_32 = (uint32_t) ((unsigned long) (func) >> 32) & 0xFFFFFFFF;
}

void IDT_init_function_pointers(struct Idt *idt){
    IDT_entry_function_pointer_init(&(idt->entries[0]), &isr_0);
    IDT_entry_function_pointer_init(&(idt->entries[1]), &isr_1);
    IDT_entry_function_pointer_init(&(idt->entries[2]), &isr_2);
    IDT_entry_function_pointer_init(&(idt->entries[3]), &isr_3);
    IDT_entry_function_pointer_init(&(idt->entries[4]), &isr_4);
    IDT_entry_function_pointer_init(&(idt->entries[5]), &isr_5);
    IDT_entry_function_pointer_init(&(idt->entries[6]), &isr_6);
    IDT_entry_function_pointer_init(&(idt->entries[7]), &isr_7);
    IDT_entry_function_pointer_init(&(idt->entries[8]), &isr_err_8);
    IDT_entry_function_pointer_init(&(idt->entries[9]), &isr_9);
    IDT_entry_function_pointer_init(&(idt->entries[10]), &isr_err_10);
    IDT_entry_function_pointer_init(&(idt->entries[11]), &isr_err_11);
    IDT_entry_function_pointer_init(&(idt->entries[12]), &isr_err_12);
    IDT_entry_function_pointer_init(&(idt->entries[13]), &isr_err_13);
    IDT_entry_function_pointer_init(&(idt->entries[14]), &isr_err_14);
    IDT_entry_function_pointer_init(&(idt->entries[15]), &isr_15);
    IDT_entry_function_pointer_init(&(idt->entries[16]), &isr_16);
    IDT_entry_function_pointer_init(&(idt->entries[17]), &isr_err_17);
    IDT_entry_function_pointer_init(&(idt->entries[18]), &isr_18);
    IDT_entry_function_pointer_init(&(idt->entries[19]), &isr_19);
    IDT_entry_function_pointer_init(&(idt->entries[20]), &isr_20);
    IDT_entry_function_pointer_init(&(idt->entries[21]), &isr_21);
    IDT_entry_function_pointer_init(&(idt->entries[22]), &isr_22);
    IDT_entry_function_pointer_init(&(idt->entries[23]), &isr_23);
    IDT_entry_function_pointer_init(&(idt->entries[24]), &isr_24);
    IDT_entry_function_pointer_init(&(idt->entries[25]), &isr_25);
    IDT_entry_function_pointer_init(&(idt->entries[26]), &isr_26);
    IDT_entry_function_pointer_init(&(idt->entries[27]), &isr_27);
    IDT_entry_function_pointer_init(&(idt->entries[28]), &isr_28);
    IDT_entry_function_pointer_init(&(idt->entries[29]), &isr_29);
    IDT_entry_function_pointer_init(&(idt->entries[30]), &isr_30);
    IDT_entry_function_pointer_init(&(idt->entries[31]), &isr_31);
    IDT_entry_function_pointer_init(&(idt->entries[32]), &isr_32);
    IDT_entry_function_pointer_init(&(idt->entries[33]), &isr_33);
    IDT_entry_function_pointer_init(&(idt->entries[34]), &isr_34);
    IDT_entry_function_pointer_init(&(idt->entries[35]), &isr_35);
    IDT_entry_function_pointer_init(&(idt->entries[36]), &isr_36);
    IDT_entry_function_pointer_init(&(idt->entries[37]), &isr_37);
    IDT_entry_function_pointer_init(&(idt->entries[38]), &isr_38);
    IDT_entry_function_pointer_init(&(idt->entries[39]), &isr_39);
    IDT_entry_function_pointer_init(&(idt->entries[40]), &isr_40);
    IDT_entry_function_pointer_init(&(idt->entries[41]), &isr_41);
    IDT_entry_function_pointer_init(&(idt->entries[42]), &isr_42);
    IDT_entry_function_pointer_init(&(idt->entries[43]), &isr_43);
    IDT_entry_function_pointer_init(&(idt->entries[44]), &isr_44);
    IDT_entry_function_pointer_init(&(idt->entries[45]), &isr_45);
    IDT_entry_function_pointer_init(&(idt->entries[46]), &isr_46);
    IDT_entry_function_pointer_init(&(idt->entries[47]), &isr_47);
    IDT_entry_function_pointer_init(&(idt->entries[48]), &isr_48);
    IDT_entry_function_pointer_init(&(idt->entries[49]), &isr_49);
    IDT_entry_function_pointer_init(&(idt->entries[50]), &isr_50);
    IDT_entry_function_pointer_init(&(idt->entries[51]), &isr_51);
    IDT_entry_function_pointer_init(&(idt->entries[52]), &isr_52);
    IDT_entry_function_pointer_init(&(idt->entries[53]), &isr_53);
    IDT_entry_function_pointer_init(&(idt->entries[54]), &isr_54);
    IDT_entry_function_pointer_init(&(idt->entries[55]), &isr_55);
    IDT_entry_function_pointer_init(&(idt->entries[56]), &isr_56);
    IDT_entry_function_pointer_init(&(idt->entries[57]), &isr_57);
    IDT_entry_function_pointer_init(&(idt->entries[58]), &isr_58);
    IDT_entry_function_pointer_init(&(idt->entries[59]), &isr_59);
    IDT_entry_function_pointer_init(&(idt->entries[60]), &isr_60);
    IDT_entry_function_pointer_init(&(idt->entries[61]), &isr_61);
    IDT_entry_function_pointer_init(&(idt->entries[62]), &isr_62);
    IDT_entry_function_pointer_init(&(idt->entries[63]), &isr_63);
    IDT_entry_function_pointer_init(&(idt->entries[64]), &isr_64);
    IDT_entry_function_pointer_init(&(idt->entries[65]), &isr_65);
    IDT_entry_function_pointer_init(&(idt->entries[66]), &isr_66);
    IDT_entry_function_pointer_init(&(idt->entries[67]), &isr_67);
    IDT_entry_function_pointer_init(&(idt->entries[68]), &isr_68);
    IDT_entry_function_pointer_init(&(idt->entries[69]), &isr_69);
    IDT_entry_function_pointer_init(&(idt->entries[70]), &isr_70);
    IDT_entry_function_pointer_init(&(idt->entries[71]), &isr_71);
    IDT_entry_function_pointer_init(&(idt->entries[72]), &isr_72);
    IDT_entry_function_pointer_init(&(idt->entries[73]), &isr_73);
    IDT_entry_function_pointer_init(&(idt->entries[74]), &isr_74);
    IDT_entry_function_pointer_init(&(idt->entries[75]), &isr_75);
    IDT_entry_function_pointer_init(&(idt->entries[76]), &isr_76);
    IDT_entry_function_pointer_init(&(idt->entries[77]), &isr_77);
    IDT_entry_function_pointer_init(&(idt->entries[78]), &isr_78);
    IDT_entry_function_pointer_init(&(idt->entries[79]), &isr_79);
    IDT_entry_function_pointer_init(&(idt->entries[80]), &isr_80);
    IDT_entry_function_pointer_init(&(idt->entries[81]), &isr_81);
    IDT_entry_function_pointer_init(&(idt->entries[82]), &isr_82);
    IDT_entry_function_pointer_init(&(idt->entries[83]), &isr_83);
    IDT_entry_function_pointer_init(&(idt->entries[84]), &isr_84);
    IDT_entry_function_pointer_init(&(idt->entries[85]), &isr_85);
    IDT_entry_function_pointer_init(&(idt->entries[86]), &isr_86);
    IDT_entry_function_pointer_init(&(idt->entries[87]), &isr_87);
    IDT_entry_function_pointer_init(&(idt->entries[88]), &isr_88);
    IDT_entry_function_pointer_init(&(idt->entries[89]), &isr_89);
    IDT_entry_function_pointer_init(&(idt->entries[90]), &isr_90);
    IDT_entry_function_pointer_init(&(idt->entries[91]), &isr_91);
    IDT_entry_function_pointer_init(&(idt->entries[92]), &isr_92);
    IDT_entry_function_pointer_init(&(idt->entries[93]), &isr_93);
    IDT_entry_function_pointer_init(&(idt->entries[94]), &isr_94);
    IDT_entry_function_pointer_init(&(idt->entries[95]), &isr_95);
    IDT_entry_function_pointer_init(&(idt->entries[96]), &isr_96);
    IDT_entry_function_pointer_init(&(idt->entries[97]), &isr_97);
    IDT_entry_function_pointer_init(&(idt->entries[98]), &isr_98);
    IDT_entry_function_pointer_init(&(idt->entries[99]), &isr_99);
    IDT_entry_function_pointer_init(&(idt->entries[100]), &isr_100);
    IDT_entry_function_pointer_init(&(idt->entries[101]), &isr_101);
    IDT_entry_function_pointer_init(&(idt->entries[102]), &isr_102);
    IDT_entry_function_pointer_init(&(idt->entries[103]), &isr_103);
    IDT_entry_function_pointer_init(&(idt->entries[104]), &isr_104);
    IDT_entry_function_pointer_init(&(idt->entries[105]), &isr_105);
    IDT_entry_function_pointer_init(&(idt->entries[106]), &isr_106);
    IDT_entry_function_pointer_init(&(idt->entries[107]), &isr_107);
    IDT_entry_function_pointer_init(&(idt->entries[108]), &isr_108);
    IDT_entry_function_pointer_init(&(idt->entries[109]), &isr_109);
    IDT_entry_function_pointer_init(&(idt->entries[110]), &isr_110);
    IDT_entry_function_pointer_init(&(idt->entries[111]), &isr_111);
    IDT_entry_function_pointer_init(&(idt->entries[112]), &isr_112);
    IDT_entry_function_pointer_init(&(idt->entries[113]), &isr_113);
    IDT_entry_function_pointer_init(&(idt->entries[114]), &isr_114);
    IDT_entry_function_pointer_init(&(idt->entries[115]), &isr_115);
    IDT_entry_function_pointer_init(&(idt->entries[116]), &isr_116);
    IDT_entry_function_pointer_init(&(idt->entries[117]), &isr_117);
    IDT_entry_function_pointer_init(&(idt->entries[118]), &isr_118);
    IDT_entry_function_pointer_init(&(idt->entries[119]), &isr_119);
    IDT_entry_function_pointer_init(&(idt->entries[120]), &isr_120);
    IDT_entry_function_pointer_init(&(idt->entries[121]), &isr_121);
    IDT_entry_function_pointer_init(&(idt->entries[122]), &isr_122);
    IDT_entry_function_pointer_init(&(idt->entries[123]), &isr_123);
    IDT_entry_function_pointer_init(&(idt->entries[124]), &isr_124);
    IDT_entry_function_pointer_init(&(idt->entries[125]), &isr_125);
    IDT_entry_function_pointer_init(&(idt->entries[126]), &isr_126);
    IDT_entry_function_pointer_init(&(idt->entries[127]), &isr_127);
    IDT_entry_function_pointer_init(&(idt->entries[128]), &isr_128);
    IDT_entry_function_pointer_init(&(idt->entries[129]), &isr_129);
    IDT_entry_function_pointer_init(&(idt->entries[130]), &isr_130);
    IDT_entry_function_pointer_init(&(idt->entries[131]), &isr_131);
    IDT_entry_function_pointer_init(&(idt->entries[132]), &isr_132);
    IDT_entry_function_pointer_init(&(idt->entries[133]), &isr_133);
    IDT_entry_function_pointer_init(&(idt->entries[134]), &isr_134);
    IDT_entry_function_pointer_init(&(idt->entries[135]), &isr_135);
    IDT_entry_function_pointer_init(&(idt->entries[136]), &isr_136);
    IDT_entry_function_pointer_init(&(idt->entries[137]), &isr_137);
    IDT_entry_function_pointer_init(&(idt->entries[138]), &isr_138);
    IDT_entry_function_pointer_init(&(idt->entries[139]), &isr_139);
    IDT_entry_function_pointer_init(&(idt->entries[140]), &isr_140);
    IDT_entry_function_pointer_init(&(idt->entries[141]), &isr_141);
    IDT_entry_function_pointer_init(&(idt->entries[142]), &isr_142);
    IDT_entry_function_pointer_init(&(idt->entries[143]), &isr_143);
    IDT_entry_function_pointer_init(&(idt->entries[144]), &isr_144);
    IDT_entry_function_pointer_init(&(idt->entries[145]), &isr_145);
    IDT_entry_function_pointer_init(&(idt->entries[146]), &isr_146);
    IDT_entry_function_pointer_init(&(idt->entries[147]), &isr_147);
    IDT_entry_function_pointer_init(&(idt->entries[148]), &isr_148);
    IDT_entry_function_pointer_init(&(idt->entries[149]), &isr_149);
    IDT_entry_function_pointer_init(&(idt->entries[150]), &isr_150);
    IDT_entry_function_pointer_init(&(idt->entries[151]), &isr_151);
    IDT_entry_function_pointer_init(&(idt->entries[152]), &isr_152);
    IDT_entry_function_pointer_init(&(idt->entries[153]), &isr_153);
    IDT_entry_function_pointer_init(&(idt->entries[154]), &isr_154);
    IDT_entry_function_pointer_init(&(idt->entries[155]), &isr_155);
    IDT_entry_function_pointer_init(&(idt->entries[156]), &isr_156);
    IDT_entry_function_pointer_init(&(idt->entries[157]), &isr_157);
    IDT_entry_function_pointer_init(&(idt->entries[158]), &isr_158);
    IDT_entry_function_pointer_init(&(idt->entries[159]), &isr_159);
    IDT_entry_function_pointer_init(&(idt->entries[160]), &isr_160);
    IDT_entry_function_pointer_init(&(idt->entries[161]), &isr_161);
    IDT_entry_function_pointer_init(&(idt->entries[162]), &isr_162);
    IDT_entry_function_pointer_init(&(idt->entries[163]), &isr_163);
    IDT_entry_function_pointer_init(&(idt->entries[164]), &isr_164);
    IDT_entry_function_pointer_init(&(idt->entries[165]), &isr_165);
    IDT_entry_function_pointer_init(&(idt->entries[166]), &isr_166);
    IDT_entry_function_pointer_init(&(idt->entries[167]), &isr_167);
    IDT_entry_function_pointer_init(&(idt->entries[168]), &isr_168);
    IDT_entry_function_pointer_init(&(idt->entries[169]), &isr_169);
    IDT_entry_function_pointer_init(&(idt->entries[170]), &isr_170);
    IDT_entry_function_pointer_init(&(idt->entries[171]), &isr_171);
    IDT_entry_function_pointer_init(&(idt->entries[172]), &isr_172);
    IDT_entry_function_pointer_init(&(idt->entries[173]), &isr_173);
    IDT_entry_function_pointer_init(&(idt->entries[174]), &isr_174);
    IDT_entry_function_pointer_init(&(idt->entries[175]), &isr_175);
    IDT_entry_function_pointer_init(&(idt->entries[176]), &isr_176);
    IDT_entry_function_pointer_init(&(idt->entries[177]), &isr_177);
    IDT_entry_function_pointer_init(&(idt->entries[178]), &isr_178);
    IDT_entry_function_pointer_init(&(idt->entries[179]), &isr_179);
    IDT_entry_function_pointer_init(&(idt->entries[180]), &isr_180);
    IDT_entry_function_pointer_init(&(idt->entries[181]), &isr_181);
    IDT_entry_function_pointer_init(&(idt->entries[182]), &isr_182);
    IDT_entry_function_pointer_init(&(idt->entries[183]), &isr_183);
    IDT_entry_function_pointer_init(&(idt->entries[184]), &isr_184);
    IDT_entry_function_pointer_init(&(idt->entries[185]), &isr_185);
    IDT_entry_function_pointer_init(&(idt->entries[186]), &isr_186);
    IDT_entry_function_pointer_init(&(idt->entries[187]), &isr_187);
    IDT_entry_function_pointer_init(&(idt->entries[188]), &isr_188);
    IDT_entry_function_pointer_init(&(idt->entries[189]), &isr_189);
    IDT_entry_function_pointer_init(&(idt->entries[190]), &isr_190);
    IDT_entry_function_pointer_init(&(idt->entries[191]), &isr_191);
    IDT_entry_function_pointer_init(&(idt->entries[192]), &isr_192);
    IDT_entry_function_pointer_init(&(idt->entries[193]), &isr_193);
    IDT_entry_function_pointer_init(&(idt->entries[194]), &isr_194);
    IDT_entry_function_pointer_init(&(idt->entries[195]), &isr_195);
    IDT_entry_function_pointer_init(&(idt->entries[196]), &isr_196);
    IDT_entry_function_pointer_init(&(idt->entries[197]), &isr_197);
    IDT_entry_function_pointer_init(&(idt->entries[198]), &isr_198);
    IDT_entry_function_pointer_init(&(idt->entries[199]), &isr_199);
    IDT_entry_function_pointer_init(&(idt->entries[200]), &isr_200);
    IDT_entry_function_pointer_init(&(idt->entries[201]), &isr_201);
    IDT_entry_function_pointer_init(&(idt->entries[202]), &isr_202);
    IDT_entry_function_pointer_init(&(idt->entries[203]), &isr_203);
    IDT_entry_function_pointer_init(&(idt->entries[204]), &isr_204);
    IDT_entry_function_pointer_init(&(idt->entries[205]), &isr_205);
    IDT_entry_function_pointer_init(&(idt->entries[206]), &isr_206);
    IDT_entry_function_pointer_init(&(idt->entries[207]), &isr_207);
    IDT_entry_function_pointer_init(&(idt->entries[208]), &isr_208);
    IDT_entry_function_pointer_init(&(idt->entries[209]), &isr_209);
    IDT_entry_function_pointer_init(&(idt->entries[210]), &isr_210);
    IDT_entry_function_pointer_init(&(idt->entries[211]), &isr_211);
    IDT_entry_function_pointer_init(&(idt->entries[212]), &isr_212);
    IDT_entry_function_pointer_init(&(idt->entries[213]), &isr_213);
    IDT_entry_function_pointer_init(&(idt->entries[214]), &isr_214);
    IDT_entry_function_pointer_init(&(idt->entries[215]), &isr_215);
    IDT_entry_function_pointer_init(&(idt->entries[216]), &isr_216);
    IDT_entry_function_pointer_init(&(idt->entries[217]), &isr_217);
    IDT_entry_function_pointer_init(&(idt->entries[218]), &isr_218);
    IDT_entry_function_pointer_init(&(idt->entries[219]), &isr_219);
    IDT_entry_function_pointer_init(&(idt->entries[220]), &isr_220);
    IDT_entry_function_pointer_init(&(idt->entries[221]), &isr_221);
    IDT_entry_function_pointer_init(&(idt->entries[222]), &isr_222);
    IDT_entry_function_pointer_init(&(idt->entries[223]), &isr_223);
    IDT_entry_function_pointer_init(&(idt->entries[224]), &isr_224);
    IDT_entry_function_pointer_init(&(idt->entries[225]), &isr_225);
    IDT_entry_function_pointer_init(&(idt->entries[226]), &isr_226);
    IDT_entry_function_pointer_init(&(idt->entries[227]), &isr_227);
    IDT_entry_function_pointer_init(&(idt->entries[228]), &isr_228);
    IDT_entry_function_pointer_init(&(idt->entries[229]), &isr_229);
    IDT_entry_function_pointer_init(&(idt->entries[230]), &isr_230);
    IDT_entry_function_pointer_init(&(idt->entries[231]), &isr_231);
    IDT_entry_function_pointer_init(&(idt->entries[232]), &isr_232);
    IDT_entry_function_pointer_init(&(idt->entries[233]), &isr_233);
    IDT_entry_function_pointer_init(&(idt->entries[234]), &isr_234);
    IDT_entry_function_pointer_init(&(idt->entries[235]), &isr_235);
    IDT_entry_function_pointer_init(&(idt->entries[236]), &isr_236);
    IDT_entry_function_pointer_init(&(idt->entries[237]), &isr_237);
    IDT_entry_function_pointer_init(&(idt->entries[238]), &isr_238);
    IDT_entry_function_pointer_init(&(idt->entries[239]), &isr_239);
    IDT_entry_function_pointer_init(&(idt->entries[240]), &isr_240);
    IDT_entry_function_pointer_init(&(idt->entries[241]), &isr_241);
    IDT_entry_function_pointer_init(&(idt->entries[242]), &isr_242);
    IDT_entry_function_pointer_init(&(idt->entries[243]), &isr_243);
    IDT_entry_function_pointer_init(&(idt->entries[244]), &isr_244);
    IDT_entry_function_pointer_init(&(idt->entries[245]), &isr_245);
    IDT_entry_function_pointer_init(&(idt->entries[246]), &isr_246);
    IDT_entry_function_pointer_init(&(idt->entries[247]), &isr_247);
    IDT_entry_function_pointer_init(&(idt->entries[248]), &isr_248);
    IDT_entry_function_pointer_init(&(idt->entries[249]), &isr_249);
    IDT_entry_function_pointer_init(&(idt->entries[250]), &isr_250);
    IDT_entry_function_pointer_init(&(idt->entries[251]), &isr_251);
    IDT_entry_function_pointer_init(&(idt->entries[252]), &isr_252);
    IDT_entry_function_pointer_init(&(idt->entries[253]), &isr_253);
    IDT_entry_function_pointer_init(&(idt->entries[254]), &isr_254);
    IDT_entry_function_pointer_init(&(idt->entries[255]), &isr_255);
}

void IDT_init(struct Idt *idt){

    for(int i = 0; i < INTERRUPT_COUNT; i++){
        // always use interrupt gate not trap gate
        initialize_options(&(idt->entries[i]), 0);
        idt->entries[i].gdt_selector = KERNEL_SEGMENT_OFFSET;
        idt->entries[i].reserved = 0;
    }
    idt->entries[DF].options.ist = 1;
    idt->entries[PF].options.ist = 2;
    idt->entries[GP].options.ist = 3;
    // initialize function pointers to isr from asm file
    IDT_init_function_pointers(idt); 

    // this table pointer struct is required by lidt to load idt
    static struct{
        uint16_t table_limit;
        void * table_address;
    }__attribute__((packed)) table_pointer; 
    table_pointer.table_limit = (uint16_t) sizeof(struct Idt) - 1;
    table_pointer.table_address = (void *) idt;

    asm volatile("lidt %0" : :"m" (table_pointer)); // load idt
}

void IDT_load(struct Idt *idt){
    return;    
}