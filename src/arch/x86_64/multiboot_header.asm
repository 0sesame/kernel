section .multiboot_header
header_start:
    dd 0xe85250d6                   ; magic number for multiboot 2
    dd 0                            ; architecture 0
    dd header_end - header_start    ; length of header
    ; checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    dw 0 ; type
    dw 0 ; flags
    dd 8 ; size
header_end:
