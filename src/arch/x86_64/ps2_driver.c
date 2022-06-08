#include <stdint-gcc.h>
#include "mystdio.h"
#include "VGA_driver.h"
#include "interrupt.h"
#include "KBD_driver.h"

#define PS2_DATA 0x60
#define PS2_CMD 0x64 
#define PS2_STATUS PS2_CMD
#define PS2_IN_STATUS 1 << 1
#define PS2_OUT_STATUS 1

static uint8_t scan_code_to_key[0xFF];
static uint8_t shift_scan_code_to_key[0xFF];

#define SHIFT_DOWN 1
#define CAPS_ON 1 << 1
#define CTRL_DOWN 1 << 2
#define ALT_DOWN 1 << 3
// 0 bit - set if shift down // 1 bit - set if caps-lock on
// 2 bit - set if ctrl held // 3 bit - set if alt is down
static uint8_t keyboard_state = 0;

#define KEY_RELEASE_OFFSET 0x80
#define TOP_OF_CHAR_RANGE 0x59

void initialize_ps2_controller(void){
    CLI;
    uint8_t configuration;
    outb(PS2_CMD, 0xAD); // disable first PS/2 port
    // wait for ps/2 device to be ready for next command
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_CMD, 0xA7); // disable second PS/2 port

    // wait for ps/2 device to be ready for next command
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_CMD, 0x20);
    // wait for config data to be ready then read it
    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){};
    configuration = inb(PS2_DATA);
    configuration = configuration | (1) | (1 << 4);
    configuration = configuration & ~(1 << 1 ) & ~(1 << 5);

    // wait for ps/2 device to be ready for next command
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};

    // write new config as 2 part command
    outb(PS2_CMD, 0x60);
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_DATA, configuration);

    // enable device 1
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_CMD, 0xAE); // disable first PS/2 port
    STI;
}

void initialize_scan_code_lookup_table(void){
    scan_code_to_key[0xB] = '0';
    for(int i=1; i<10; i++){
        scan_code_to_key[0x01 + i] = '0' + i;
    }
    scan_code_to_key[0xC] = '-'; scan_code_to_key[0xD] = '='; scan_code_to_key[0xF] = '\t';
    scan_code_to_key[0x10] = 'q'; scan_code_to_key[0x11] = 'w'; scan_code_to_key[0x12] = 'e';
    scan_code_to_key[0x13] = 'r'; scan_code_to_key[0x14] = 't'; scan_code_to_key[0x15] = 'y';
    scan_code_to_key[0x16] = 'u'; scan_code_to_key[0x17] = 'i'; scan_code_to_key[0x18] = 'o';
    scan_code_to_key[0x19] = 'p'; scan_code_to_key[0x1A] = '['; scan_code_to_key[0x1b] = ']';
    scan_code_to_key[0x1c] = '\n'; scan_code_to_key[0x1e] = 'a';
    scan_code_to_key[0x1f] = 's'; scan_code_to_key[0x20] = 'd'; scan_code_to_key[0x21] = 'f';
    scan_code_to_key[0x22] = 'g'; scan_code_to_key[0x23] = 'h'; scan_code_to_key[0x24] = 'j';
    scan_code_to_key[0x25] = 'k'; scan_code_to_key[0x26] = 'l'; scan_code_to_key[0x27] = ';';
    scan_code_to_key[0x28] = '\''; scan_code_to_key[0x29] = '`';
    scan_code_to_key[0x2B] = '\\'; scan_code_to_key[0x2C] = 'z'; scan_code_to_key[0x2D] = 'x';
    scan_code_to_key[0x2E] = 'c'; scan_code_to_key[0x2F] = 'v'; scan_code_to_key[0x30] = 'b';
    scan_code_to_key[0x31] = 'n'; scan_code_to_key[0x32] = 'm'; scan_code_to_key[0x33] = ',';
    scan_code_to_key[0x34] = '.'; scan_code_to_key[0x35] = '/'; scan_code_to_key[0x39] = ' ';

    shift_scan_code_to_key[0xB] = ')'; shift_scan_code_to_key[0x2] = '!'; shift_scan_code_to_key[0x3] = '@';
    shift_scan_code_to_key[0x4] = '#'; shift_scan_code_to_key[0x5] = '$'; shift_scan_code_to_key[0x6] = '%';
    shift_scan_code_to_key[0x7] = '^'; shift_scan_code_to_key[0x8] = '&'; shift_scan_code_to_key[0x9] = '*';
    shift_scan_code_to_key[0xA] = '('; shift_scan_code_to_key[0xC] = '_'; shift_scan_code_to_key[0xD] = '+';
    shift_scan_code_to_key[0x10] = 'Q'; shift_scan_code_to_key[0x11] = 'W'; shift_scan_code_to_key[0x12] = 'E';
    shift_scan_code_to_key[0x13] = 'R'; shift_scan_code_to_key[0x14] = 'T'; shift_scan_code_to_key[0x15] = 'Y';
    shift_scan_code_to_key[0x16] = 'U'; shift_scan_code_to_key[0x17] = 'I'; shift_scan_code_to_key[0x18] = 'O';
    shift_scan_code_to_key[0x19] = 'P'; shift_scan_code_to_key[0x1A] = '{'; shift_scan_code_to_key[0x1b] = '}';
    shift_scan_code_to_key[0x1c] = '\n'; shift_scan_code_to_key[0x1e] = 'A';
    shift_scan_code_to_key[0x1f] = 'S'; shift_scan_code_to_key[0x20] = 'D'; shift_scan_code_to_key[0x21] = 'F';
    shift_scan_code_to_key[0x22] = 'G'; shift_scan_code_to_key[0x23] = 'H'; shift_scan_code_to_key[0x24] = 'J';
    shift_scan_code_to_key[0x25] = 'K'; shift_scan_code_to_key[0x26] = 'L'; shift_scan_code_to_key[0x27] = ':';
    shift_scan_code_to_key[0x28] = '"'; shift_scan_code_to_key[0x29] = '~';
    shift_scan_code_to_key[0x2B] = '|'; shift_scan_code_to_key[0x2C] = 'Z'; shift_scan_code_to_key[0x2D] = 'X';
    shift_scan_code_to_key[0x2E] = 'C'; shift_scan_code_to_key[0x2F] = 'V'; shift_scan_code_to_key[0x30] = 'B';
    shift_scan_code_to_key[0x31] = 'N'; shift_scan_code_to_key[0x32] = 'M'; shift_scan_code_to_key[0x33] = '<';
    shift_scan_code_to_key[0x34] = '>'; shift_scan_code_to_key[0x35] = '?';
}

void initialize_ps2_keyboard(void){
    CLI;
    uint8_t data;
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_DATA, 0xFF); // send rest to keyboard

    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){};
    if((data = inb(PS2_DATA)) != 0xFA){ // get result of reset
        printk("Keyboard did not ack reset request (code %x)\n", data);
    }    
    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){};
    if((data = inb(PS2_DATA)) != 0xAA){ // get result of reset
        printk("Keyboard did not pass self-test (code %x)\n", data);
    }    
    
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_DATA, 0xF0); // command for setting scan code
    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){};
    if((data = inb(PS2_DATA)) != 0xFA){ // get result of reset
        printk("Keyboard did not ack scan code request (code %x)\n", data);
    }    
    while(inb(PS2_STATUS) & PS2_IN_STATUS){};
    outb(PS2_DATA, 0x02); // set scan code set to 2

    initialize_scan_code_lookup_table();
    STI;
}

char get_ascii_from_key_press(uint8_t data){
    // assumes valid range key press for state change or ascii out
    if(data == 0x2A || data == 0x36){ // get all state changers
        keyboard_state = keyboard_state | SHIFT_DOWN;
        return '\0';
    }
    else if(data == 0x1D){
        keyboard_state = keyboard_state | CTRL_DOWN;
        return '\0';
    }
    else if(data == 0x38){
        keyboard_state = keyboard_state | ALT_DOWN;
        return '\0';
    }
    else if(data == 0x3A){
        if(keyboard_state & CAPS_ON){
            keyboard_state = keyboard_state & ~(CAPS_ON);
            return '\0';
        }
        else{
            keyboard_state = keyboard_state | CAPS_ON;
            return '\0';
        }
    }
    else if(data == 0x0E){
        VGA_remove_char();
    }
    else if(data <= 0x39){ // all key-down presses we want to display or use
        if(keyboard_state & SHIFT_DOWN){
            return shift_scan_code_to_key[data];
        }
        else if(keyboard_state & CAPS_ON){
            // if alpha, use upper case from shift lookup table 
            if(shift_scan_code_to_key[data] >= 'A' && shift_scan_code_to_key[data] <= 'Z'){ 
                return shift_scan_code_to_key[data];
            }
            // if not alpha, use normal key from lookup table
            else{
                return scan_code_to_key[data];
            }
        }
        else{
            return scan_code_to_key[data];
        }
    }
    return '\0';
}

void handle_key_release(uint8_t data){
    // uses released keys to change state
    data = data - KEY_RELEASE_OFFSET;
    if(data == 0x2A || data == 0x36){
        keyboard_state = keyboard_state & ~(SHIFT_DOWN);
    }
    else if(data == 0x1D){
        keyboard_state = keyboard_state & ~(CTRL_DOWN);
    }
    else if(data == 0x38){
        keyboard_state = keyboard_state & ~(ALT_DOWN);
    }
}

void get_and_display_key_press(void){
    // meant to be used in loop
    uint8_t data;
    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){};// wait for input
    data = inb(PS2_DATA);
    if(data < TOP_OF_CHAR_RANGE){
        KBD_write(get_ascii_from_key_press(data));
    }else if(data >= KEY_RELEASE_OFFSET){
        handle_key_release(data);
    }
}

char get_key_press(void){
    // returns pressed character if displayable character is pressed
    // else returns nul character
    uint8_t data;
    char ascii;
    while(!(inb(PS2_STATUS) & PS2_OUT_STATUS)){}
    data = inb(PS2_DATA);
    if(data < TOP_OF_CHAR_RANGE){
        if((ascii = get_ascii_from_key_press(data))){
            return get_ascii_from_key_press(data);
        }
    }else if(data >= KEY_RELEASE_OFFSET){
        handle_key_release(data);
    }
    return '\0'; // return nul when no ascii character present
}
