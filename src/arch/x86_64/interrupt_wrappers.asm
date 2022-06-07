%macro ISR 1
global isr_%1
isr_%1:
    push rdi   ; no error code this time
    push rsi   ; no error code, but keep stack same for generic isr call
    ;sub rsp, 8 ; set new stack pointer below pushed values
    mov rdi, %1; put interrupt number in rdi
    jmp generic_isr
    ; stack after:
    ; ... see bellardo notes or osdev for rest of stack
    ; Return RIP
    ; rdi
    ; rsi
    ; uninit    <-- rsp
%endmacro

%macro ISR_ERR 1
global isr_err_%1
isr_err_%1:
    push rsi
    mov rsi, [rsp + 8] ; puts error code in rsi
    mov [rsp + 8], rdi ; puts rdi where error code was
    ;sub rsp, 8         ; update rsp to be below arguments
    mov rdi, %1        ; put interrupt # in rdi
    jmp generic_isr    ; go to generic interrupt handler
    ; stack after:
    ; ... see bellardo notes or osdev for rest of stack
    ; Return RIP
    ; rdi
    ; rsi
    ; uninit    <-- rsp
%endmacro

struc thread_ctx
    .rsi: resq 1
    .rdi: resq 1
    .rdx: resq 1
    .rcx: resq 1
    .rbx: resq 1
    .rax: resq 1
    .r8: resq 1
    .r9: resq 1
    .r10: resq 1
    .r11: resq 1
    .r12: resq 1
    .r13: resq 1
    .r14: resq 1
    .r15: resq 1
    .rbp: resq 1
    .rsp: resq 1
    .rip: resq 1
    .rflags: resq 1
    .cs: resw 1
    .ss: resw 1
    .ds: resw 1
    .es: resw 1
    .fs: resw 1
    .gs: resw 1
endstruc

section .text
bits 64
extern handle_irq
extern curr_proc
extern next_proc
generic_isr:
    ; data registers are saved based on caller conventions

    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    cld     ; clear direction flag as per convention according to osdev wiki
    call handle_irq ; call c function for handling interrupts
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax

    ; get rsi and rdi from when we saved them
    pop rsi
    pop rdi
    
    push rax
    push rbx
    mov rax, [curr_proc]
    mov rbx, [next_proc]
    cmp rax,rbx
    jne .save_curr_ctx

    pop rbx
    pop rax
    iretq

.save_curr_ctx:
    ; pop both for my own noggin's sake
    pop rax
    pop rbx

    ; end up pushing rbx anyway but im too tired to keep track of stuff
    push rbx
    mov rbx, [curr_proc]
    mov [rbx + thread_ctx.rax], rax
    mov rax, [curr_proc]
    pop rbx
    mov [rax + thread_ctx.rbx], rbx
    mov [rax + thread_ctx.rdi], rdi
    mov [rax + thread_ctx.rsi], rsi
    mov [rax + thread_ctx.rdx], rdx
    mov [rax + thread_ctx.rcx], rcx
    mov [rax + thread_ctx.r8], r8
    mov [rax + thread_ctx.r9], r9
    mov [rax + thread_ctx.r10], r10
    mov [rax + thread_ctx.r11], r11
    mov [rax + thread_ctx.r12], r12
    mov [rax + thread_ctx.r13], r13
    mov [rax + thread_ctx.r14], r14
    mov [rax + thread_ctx.r15], r15
    mov [rax + thread_ctx.rbp], rbp

    mov rbx, [rsp]
    mov [rax + thread_ctx.rip], rbx
    mov rbx, [rsp + 8]
    mov [rax + thread_ctx.cs], rbx
    mov rbx, [rsp + 16]
    mov [rax + thread_ctx.rflags], rbx
    mov rbx, [rsp + 24]
    mov [rax + thread_ctx.rsp], rbx
    mov rbx, [rsp + 32]
    mov [rax + thread_ctx.ss], rbx

    mov [rax + thread_ctx.ds], ds
    mov [rax + thread_ctx.es], es
    mov [rax + thread_ctx.fs], fs
    mov [rax + thread_ctx.gs], gs

    mov rbx, [next_proc]
    mov [curr_proc], rbx

.restore_next_ctx:

    mov rax, [rbx + thread_ctx.rip]
    mov [rsp], rax
    mov rax, [rbx + thread_ctx.cs]
    mov [rsp + 8], rax
    mov rax, [rbx + thread_ctx.rflags]
    mov [rsp + 16], rax
    mov rax, [rbx + thread_ctx.rsp]
    mov [rsp + 24], rax
    mov rax, [rbx + thread_ctx.ss]
    mov [rsp + 32], rax

    mov rax, [rbx + thread_ctx.rax]
    mov rdi, [rbx + thread_ctx.rdi]
    mov rsi, [rbx + thread_ctx.rsi]
    mov rdx, [rbx + thread_ctx.rdx]
    mov rcx, [rbx + thread_ctx.rcx]
    mov r8, [rbx + thread_ctx.r8]
    mov r9, [rbx + thread_ctx.r9]
    mov r10, [rbx + thread_ctx.r10]
    mov r11, [rbx + thread_ctx.r11]
    mov r12, [rbx + thread_ctx.r12]
    mov r13, [rbx + thread_ctx.r13]
    mov r14, [rbx + thread_ctx.r14]
    mov r15, [rbx + thread_ctx.r15]
    mov ds, [rbx + thread_ctx.ds]
    mov es, [rbx + thread_ctx.es]
    mov fs, [rbx + thread_ctx.fs]
    mov gs, [rbx + thread_ctx.gs]
    mov rbp, [rbx + thread_ctx.rbp]
    mov rbx, [rbx + thread_ctx.rbx]
    iretq

ISR 0
ISR 1
ISR 2
ISR 3
ISR 4
ISR 5
ISR 6
ISR 7
ISR_ERR 8
ISR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR 15
ISR 16
ISR_ERR 17
ISR 18
ISR 19
ISR 20
ISR 21
ISR 22
ISR 23
ISR 24
ISR 25
ISR 26
ISR 27
ISR 28
ISR 29
ISR 30
ISR 31
ISR 32
ISR 33
ISR 34
ISR 35
ISR 36
ISR 37
ISR 38
ISR 39
ISR 40
ISR 41
ISR 42
ISR 43
ISR 44
ISR 45
ISR 46
ISR 47
ISR 48
ISR 49
ISR 50
ISR 51
ISR 52
ISR 53
ISR 54
ISR 55
ISR 56
ISR 57
ISR 58
ISR 59
ISR 60
ISR 61
ISR 62
ISR 63
ISR 64
ISR 65
ISR 66
ISR 67
ISR 68
ISR 69
ISR 70
ISR 71
ISR 72
ISR 73
ISR 74
ISR 75
ISR 76
ISR 77
ISR 78
ISR 79
ISR 80
ISR 81
ISR 82
ISR 83
ISR 84
ISR 85
ISR 86
ISR 87
ISR 88
ISR 89
ISR 90
ISR 91
ISR 92
ISR 93
ISR 94
ISR 95
ISR 96
ISR 97
ISR 98
ISR 99
ISR 100
ISR 101
ISR 102
ISR 103
ISR 104
ISR 105
ISR 106
ISR 107
ISR 108
ISR 109
ISR 110
ISR 111
ISR 112
ISR 113
ISR 114
ISR 115
ISR 116
ISR 117
ISR 118
ISR 119
ISR 120
ISR 121
ISR 122
ISR 123
ISR 124
ISR 125
ISR 126
ISR 127
ISR 128
ISR 129
ISR 130
ISR 131
ISR 132
ISR 133
ISR 134
ISR 135
ISR 136
ISR 137
ISR 138
ISR 139
ISR 140
ISR 141
ISR 142
ISR 143
ISR 144
ISR 145
ISR 146
ISR 147
ISR 148
ISR 149
ISR 150
ISR 151
ISR 152
ISR 153
ISR 154
ISR 155
ISR 156
ISR 157
ISR 158
ISR 159
ISR 160
ISR 161
ISR 162
ISR 163
ISR 164
ISR 165
ISR 166
ISR 167
ISR 168
ISR 169
ISR 170
ISR 171
ISR 172
ISR 173
ISR 174
ISR 175
ISR 176
ISR 177
ISR 178
ISR 179
ISR 180
ISR 181
ISR 182
ISR 183
ISR 184
ISR 185
ISR 186
ISR 187
ISR 188
ISR 189
ISR 190
ISR 191
ISR 192
ISR 193
ISR 194
ISR 195
ISR 196
ISR 197
ISR 198
ISR 199
ISR 200
ISR 201
ISR 202
ISR 203
ISR 204
ISR 205
ISR 206
ISR 207
ISR 208
ISR 209
ISR 210
ISR 211
ISR 212
ISR 213
ISR 214
ISR 215
ISR 216
ISR 217
ISR 218
ISR 219
ISR 220
ISR 221
ISR 222
ISR 223
ISR 224
ISR 225
ISR 226
ISR 227
ISR 228
ISR 229
ISR 230
ISR 231
ISR 232
ISR 233
ISR 234
ISR 235
ISR 236
ISR 237
ISR 238
ISR 239
ISR 240
ISR 241
ISR 242
ISR 243
ISR 244
ISR 245
ISR 246
ISR 247
ISR 248
ISR 249
ISR 250
ISR 251
ISR 252
ISR 253
ISR 254
ISR 255