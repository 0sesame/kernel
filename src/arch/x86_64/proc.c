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
static struct ProcessQueue run_queue;

void proc_queue_add(struct Process *add, struct ProcessQueue *q){
    if(!q){
        q = &run_queue;
    }
    add->next = 0;
    if(!(q->head)){ // initialize queue
        q->head = add;
        q->tail = add;
        q->tail->prev = q->head;
        q->head->prev = 0x0;
    }
    else{ // add to end, set to tail to add
        add->prev = q->tail;
        q->tail->next = add;
        q->tail = add;
    }
}

void proc_remove_curr_from_run_queue(){
    if(run_queue.tail){
        if(run_queue.tail == run_queue.head){// only one proc in queue
            run_queue.head = 0x0;
            run_queue.tail = 0x0;
        }
        else{
            run_queue.tail = run_queue.tail->prev;
            run_queue.tail->next = 0x0;
        }
    }

}

struct Process *proc_queue_pop(struct ProcessQueue *q){ // remove head from front of queue
    struct Process *popped = q->head;
    if(q->head){
        q->head = q->head->next;
        q->head->prev = 0x0;
    }
    return popped;
}

struct Process *proc_queue_next(struct ProcessQueue *q){
    struct Process *ret = proc_queue_pop(q); 
    if(ret){
        proc_queue_add(ret, q); // popped head to tail
    }

    return ret;
}

void proc_queue_init(struct ProcessQueue *q){
    q->head = 0x0;
    q->tail = 0x0;
}

void PROC_init(void){
    proc_queue_init(&run_queue);
}

void PROC_run(void){
    if(!(run_queue.head)){
        return;
    }

    curr_proc = &(return_proc);
    next_proc = &(return_proc);
    STI;
    yield();
}

void PROC_destroy_running(void){
    struct Process *to_destroy = run_queue.tail;
    proc_remove_curr_from_run_queue();
    kfree(to_destroy);
}

void PROC_reschedule(void){ // called from yield
    struct Process *nxt;
    if(!(nxt = proc_queue_next(&run_queue))){
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

    proc_queue_add(new_kthread, &run_queue);
    return new_kthread;
}

void PROC_block_on(struct ProcessQueue *queue, int enable_ints){
    if(!queue){
        return;
    }
    proc_remove_curr_from_run_queue();
    proc_queue_add(curr_proc, queue);
    if(enable_ints){ // calling proc may call from int off context, need ints for yield
        STI;
    }
    yield();
}

void PROC_unblock_one(struct ProcessQueue *q){
    struct Process *proc = proc_queue_pop(q);
    if(proc){
        proc_queue_add(proc, &run_queue);
    }
}