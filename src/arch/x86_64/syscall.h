#ifndef SYSCALL_H
#define SYSCALL_H

#define YIELD_INT_NUM 0x80
#define EXIT_INT_NUM 0x81
void yield(void); // triggers interrupt 0x80
void kexit(void); // triggers interrupt 0x81

#endif
