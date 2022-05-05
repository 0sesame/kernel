#include <stdarg.h>
#include <stdint-gcc.h>
#include "mystdio.h"
#include "VGA_driver.h"
#include "serial.h"

void char_out(char out){
    // puts character out to wherever it's expected, vga display and serial out
    VGA_display_char(out);
    SER_write(&out, 1);
}

int printk(const char *fmt, ...){
// print char as we go along
// when we get to % format specifier, figure out which one it is and pass to function
    va_list va;
    va_start(va, fmt);
    int short_conversion = 0;
    int long_conversion = 0;
    while(*fmt){
        if(*fmt == '%'){
            fmt++;
            if(*fmt){
                if(*fmt == 'h'){
                    short_conversion = 1;
                    fmt++;
                }
                else if(*fmt == 'l'){
                    long_conversion = 1;
                    fmt++;
                }
                if(!(*fmt)){
                    break;
                }
                if(*fmt == '%'){
                    char_out(*fmt);
                }
                else if(*fmt == 'd'){
                    if(long_conversion){
                        print_long((long) va_arg(va, int));
                    }
                    else if(short_conversion){
                        print_int((short) va_arg(va, int));
                    }
                    else{
                        print_int(va_arg(va, int));
                    }
                }
                else if(*fmt == 'u'){
                    if(long_conversion){
                        print_unsigned_long((unsigned long) va_arg(va, unsigned int));
                    }
                    else if(short_conversion){
                        print_unsigned_short((unsigned short) va_arg(va, unsigned int));
                    }
                    else{
                        print_unsigned(va_arg(va, unsigned int));
                    }
                }
                else if(*fmt == 'x'){
                    if(long_conversion){
                        print_hex_long((unsigned long) va_arg(va, unsigned int));
                    }
                    else if(short_conversion){
                        print_hex_short((unsigned short) va_arg(va, unsigned int));
                    }
                    else{
                        print_hex(va_arg(va, unsigned int));
                    }
                }
                else if(*fmt == 'c'){
                    print_char(va_arg(va, int));
                }
                else if(*fmt == 'p'){
                    print_hex_long((unsigned long) va_arg(va, void *));
                }
                else if(*fmt == 's'){
                    print_string(va_arg(va, char *));
                }
            }
        }
        else{
            char_out(*fmt);
        }
        fmt++;
    }
    return 0;
}

void print_char(int to_print){
    char_out((unsigned char) to_print);
}

void print_string(char *str){
    while(*str){
        char_out(*str);
        str++;
    }
}

void print_hex_short(unsigned short to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 16) * 16; // get value in 1's place
        if(temp < 10){
            buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        }
        else{
            buff[i] = 'A' + temp - 10; // go into A-F if temp isn't 0-9
        }
        to_print = to_print / 16;
        i++;
    }
    i--;

    // prints characters of int from buff
    char_out('0');
    char_out('x');
    for(; i >= 0; i--){
        char_out(buff[i]);
    }
}

void print_hex_long(unsigned long to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 16) * 16; // get value in 1's place
        if(temp < 10){
            buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        }
        else{
            buff[i] = 'A' + temp - 10; // go into A-F if temp isn't 0-9
        }
        to_print = to_print / 16;
        i++;
    }
    i--;

    // prints characters of int from buff
    char_out('0');
    char_out('x');
    for(; i >= 0; i--){
        char_out(buff[i]);
    }
}

void print_hex(unsigned int to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 16) * 16; // get value in 1's place
        if(temp < 10){
            buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        }
        else{
            buff[i] = 'A' + temp - 10; // go into A-F if temp isn't 0-9
        }
        to_print = to_print / 16;
        i++;
    }
    i--;

    // prints characters of int from buff
    char_out('0');
    char_out('x');
    for(; i >= 0; i--){
        char_out(buff[i]);
    }

}

void print_unsigned_short(unsigned short to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    for(; i >= 0; i--){
        char_out(buff[i]);
    }
}

void print_unsigned(unsigned int to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    for(; i >= 0; i--){
        char_out(buff[i]);
    }
}

void print_unsigned_long(unsigned long to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    for(; i >= 0; i--){
        char_out(buff[i]);
    }
}

void print_long(long to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;
    int negative = 0;

    // handle negative case with negative flag
    if(to_print < 0){
        negative = 1;
        to_print = to_print * -1;
    }

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }

    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    if(negative){
        char_out('-');
    }
    for(; i >= 0; i--){
        char_out(buff[i]);
    }

}

void print_int(int to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;
    int negative = 0;

    // handle negative case with negative flag
    if(to_print < 0){
        negative = 1;
        to_print = to_print * -1;
    }

    if(to_print == 0){
        buff[i] = '0';
        i++;
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    if(negative){
        char_out('-');
    }
    for(; i >= 0; i--){
        char_out(buff[i]);
    }

};

void print_short(short to_print){
    char buff[32]; // just create big enough buffer to hold work in progress
    int temp;
    int i = 0;
    int negative = 0;

    // handle negative case with negative flag
    if(to_print < 0){
        negative = 1;
        to_print = to_print * -1;
    }

    if(to_print == 0){
        buff[i] = '0';
    }
    // puts characters of int into buff backwards
    while(to_print > 0){
        temp = to_print - (to_print / 10) * 10; // get value in 1's place
        buff[i] = '0' + temp; // put ascii char at 1's place offset into buff;
        to_print = to_print / 10;
        i++;
    }
    i--;

    // prints characters of int from buff
    if(negative){
        char_out('-');
    }
    for(; i >= 0; i--){
        char_out(buff[i]);
    }

};

inline uint8_t inb(uint16_t port){
    uint8_t ret;
    asm volatile("inb %1, %0"
        :"=a" (ret)
        :"Nd" (port));
    return ret;
}

inline void outb(uint16_t port, uint8_t val){
    asm volatile("outb %0, %1"
        :
        :"a"(val), "Nd"(port));
}

void io_wait(void){
    // does quick instruction as imprecise wait time for old hardware
    // 0x80 is an unused port
    outb(0x80, 0);
}