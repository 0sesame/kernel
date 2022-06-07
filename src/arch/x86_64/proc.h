#include <stdint-gcc.h>

#define PROC_THREAD_STACK_SPACING ((uint64_t) 1 << 21)
#define PROC_THREAD_STACK_PAGES 250

typedef void (*kproc_t)(void *);
struct thread_ctx{
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;
    uint16_t cs;
    uint16_t ss;
    uint16_t ds;
    uint16_t es;
    uint16_t fs;
    uint16_t gs;
}__attribute__((packed));

void PROC_reschedule(void);
void PROC_run(void);
void PROC_init(void);
void PROC_destroy_running(void);
void PROC_create_kthread(kproc_t entry_point, void *arg);
extern struct thread_ctx *curr_proc;
extern struct thread_ctx *next_proc;