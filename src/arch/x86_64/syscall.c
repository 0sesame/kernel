#include "syscall.h"

void yield(void){ // triggers interrupt 0x80
    asm volatile("int $0x80" ::);
}