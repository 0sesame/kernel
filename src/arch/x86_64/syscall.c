#include "syscall.h"

// currently using differnt interrupt nums for diff syscalls, easiest thing but not the best design
void yield(void){ // triggers interrupt 0x80
    asm volatile("int $0x80" ::);
}

void kexit(void){ // triggers interrupt 0x81
    asm volatile("int $0x81" ::);
}