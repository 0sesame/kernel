#include <stdint-gcc.h>

extern int printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern void print_int(int);
extern void print_long(long);
extern void print_short(short);
extern void print_unsigned(unsigned int to_print);
void print_unsigned_long(unsigned long to_print);
void print_unsigned_short(unsigned short to_print);
void print_hex(unsigned int to_print);
void print_hex_short(unsigned short to_print);
void print_hex_long(unsigned long to_print);
void print_char(int to_print);
void print_string(char *str);

extern uint8_t inb(uint16_t);
extern void outb(uint16_t, uint8_t);
extern void io_wait(void);
void char_out(char);