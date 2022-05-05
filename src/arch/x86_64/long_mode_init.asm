global long_mode_start

section .text
bits 64 ; 64 bit instructions yay!

long_mode_start:
    ; load 0 into all data segment registers
    ; required by some instructions, previously had 32-bit stuff
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; kernel main
    extern kmain
    call kmain
    hlt