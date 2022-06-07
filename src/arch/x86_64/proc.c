#include "proc.h"
#include "mmu.h"
#include "malloc.h"
#include "interrupt.h"
#include "syscall.h"
#include "mystdio.h"

struct Process *curr_proc;
struct Process *next_proc;

static int proc_count = 0;
static struct Process return_proc; // holds process to return to when proc_run has nothing to run
static struct Process *kthread_head;
static struct Process *kthread_tail;

void proc_queue_add(struct Process *add){
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

struct Process *proc_queue_pop(void){ // remove head from front of queue
    struct Process *popped = kthread_head;
    if(kthread_head){
        kthread_head = kthread_head->next;
        kthread_head->prev = 0x0;
    }
    return popped;
}

struct Process *proc_queue_next(void){
    struct Process *ret = proc_queue_pop(); 
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

    curr_proc = &(return_proc);
    next_proc = &(return_proc);
    yield();
}

void PROC_destroy_running(void){
    struct Process *to_destroy = kthread_tail;
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
    struct Process *nxt;
    if(!(nxt = proc_queue_next())){
        next_proc = &(return_proc);
        return;
    }

    next_proc = nxt; // set next_proc as head
}

struct Process *PROC_create_kthread(kproc_t entry_point, void *arg){
    // creates new kthread and associated data-structures
    // sets kthread as tail of linked list queue
    void *stack;
    struct Process *new_kthread = kcalloc(sizeof(struct Process));
    new_kthread->arg = arg;

    stack = MMU_alloc_kstack();
    if(!(stack)){
        kfree(new_kthread);
        printk("Failed to create kthread, stack allocation failed\n");
        return 0x0;
    }

    new_kthread->pid = proc_count++;
    new_kthread->ctx.rip = (uint64_t) entry_point;
    new_kthread->ctx.rbp = (uint64_t) stack;
    new_kthread->ctx.rsp = (uint64_t) stack;
    new_kthread->ctx.rdi = (uint64_t) arg;
    new_kthread->ctx.cs = KERNEL_SEGMENT_OFFSET;
    new_kthread->ctx.rflags = 0x2 | 0x200; // reserved 1 and int enable

    proc_queue_add(new_kthread);
    return new_kthread;
}
