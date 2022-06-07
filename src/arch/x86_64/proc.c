#include "proc.h"
#include "mmu.h"
#include "malloc.h"
#include "interrupt.h"
#include "syscall.h"
#include "mystdio.h"

struct thread_ctx *curr_proc;
struct thread_ctx *next_proc;

struct kthread{
    struct thread_ctx ctx;
    kproc_t *entry_point;
    void * arg;
    struct kthread *next;
    struct kthread *prev;
};

static struct kthread return_proc; // holds process to return to when proc_run has nothing to run
static struct kthread *kthread_head;
static struct kthread *kthread_tail;

void proc_queue_add(struct kthread *add){
    add->next = 0;
    if(!(kthread_head)){ // initialize queue
        kthread_head = add;
        kthread_tail = add;
        kthread_tail->prev = kthread_head;
        kthread_head->prev = 0x0;
    }
    else{ // add to end, set to tail to add
        add->prev = kthread_tail;
        kthread_tail->next = add;
        kthread_tail = add;
    }
}

struct kthread *proc_queue_pop(void){ // remove head from front of queue
    struct kthread *popped = kthread_head;
    if(kthread_head){
        kthread_head = kthread_head->next;
        kthread_head->prev = 0x0;
    }
    return popped;
}

struct kthread *proc_queue_next(void){
    struct kthread *ret = proc_queue_pop(); 
    if(ret){
        proc_queue_add(ret); // popped head to tail
    }

    return ret;
}

void PROC_init(void){
    kthread_head = 0x0;
    kthread_tail = 0x0;
}

void PROC_run(void){
    if(!(kthread_head)){
        return;
    }

    curr_proc = &(return_proc.ctx);
    next_proc = &(return_proc.ctx);
    yield();
}

void PROC_destroy_running(void){
    struct kthread *to_destroy = kthread_tail;
    if(kthread_tail){
        if(kthread_tail == kthread_head){// only one proc in queue
            kfree(to_destroy);
            kthread_head = 0x0;
            kthread_tail = 0x0;
        }
        else{
            kthread_tail = kthread_tail->prev;
            kthread_tail->next = 0x0;
            kfree(to_destroy);
        }
    }
}

void PROC_reschedule(void){ // called from yield
    struct kthread *nxt;
    if(!(nxt = proc_queue_next())){
        next_proc = &(return_proc.ctx);
        return;
    }

    next_proc = &(nxt->ctx); // set next_proc as head
}

void PROC_create_kthread(kproc_t entry_point, void *arg){
    // creates new kthread and associated data-structures
    // sets kthread as tail of linked list queue
    void *stack;
    struct kthread *new_kthread = kcalloc(sizeof(struct kthread));
    new_kthread->arg = arg;

    stack = MMU_alloc_kstack();
    if(!(stack)){
        kfree(new_kthread);
        printk("Failed to create kthread, stack allocation failed\n");
        return;
    }
    printk("alloc'd stack %p\n", stack);

    new_kthread->ctx.rip = (uint64_t) entry_point;
    new_kthread->ctx.rbp = (uint64_t) stack;
    new_kthread->ctx.rsp = (uint64_t) stack;
    new_kthread->ctx.rdi = (uint64_t) arg;
    new_kthread->ctx.cs = KERNEL_SEGMENT_OFFSET;
    new_kthread->ctx.rflags = 0x2 | 0x200; // reserved 1 and int enable

    proc_queue_add(new_kthread);
}
