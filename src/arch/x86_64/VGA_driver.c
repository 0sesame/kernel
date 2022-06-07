#include "string.h"
#include "interrupt.h"
#include "mystdio.h"
#include "VGA_driver.h"

static unsigned short *vgaBuff = (unsigned short *) 0xb8000;
static unsigned short width = 80;
static unsigned short height = 25;
static int cursor = 0;
static unsigned char default_color = VGA_COLOR(VGA_WHITE, VGA_BLACK);

int VGA_row_count(void){
    return width;
}

int VGA_col_count(void){
    return height;
}

void VGA_display_char_at(unsigned short row, unsigned short column, char c, char color){
    /*
    int enable_ints = 0;
    if(are_interrupts_enabled()){
        enable_ints = 1;
        CLI;
    }
    */
    CLI_IF;
    unsigned short buffer_offset = (row % height) * width + (column % width);
    unsigned short to_write = c | (color << 8);
    vgaBuff[buffer_offset] = to_write;
    STI_IF;
    /*
    if(enable_ints){
        STI;
    }
    */
}

void VGA_display_attr_char(int x, int y, char c, int fg, int bg){
    unsigned short color = VGA_COLOR(fg, bg);
    VGA_display_char_at((unsigned short) x, (unsigned short) y, c, color);
}

void scroll(void){
    memset(vgaBuff, VGA_BLACK, 2*sizeof(unsigned char) * width);
    for(int row = 0; row < height - 1; row++){
        memcpy(vgaBuff + (row * width), vgaBuff + ((row + 1) * width), 2*sizeof(unsigned char) * width);
    }
    memset(vgaBuff + (height - 1) * width, '\0', 2*sizeof(unsigned char) * width);
    cursor = (height - 1) * 80;
}

void move_cursor_once(void){
    cursor = cursor + 1;
    if(cursor >= width * height){
        scroll();
    }
}

void move_cursor_new_line(void){
    int current_line = cursor / width;
    cursor = (current_line + 1) * width;
    if(cursor >= width * height){
        scroll();
    }
}

void VGA_display_char(char c){
    /*
    int enable_ints = 0;
    if(are_interrupts_enabled()){
        enable_ints = 1;
        CLI;
    }
    */
    CLI_IF;
    if(c == '\n'){
        move_cursor_new_line();
        return;
    }
    vgaBuff[cursor] = c | (default_color << 8);
    move_cursor_once();
    STI_IF;
    /*
    if(enable_ints){
        STI;
    }
    */
}

void VGA_display_char_color(char c, char color){
    // WARN: Not protecting global color here, could mess with interrupt
    unsigned char color_holder = default_color;
    default_color = color;
    VGA_display_char(c);
    default_color = color_holder;
}

void VGA_display_str(const char *str){
    while(*str){
        VGA_display_char(*str);
        str++;
    }
}

void VGA_remove_char(void){
    cursor = cursor - 1;
    if(cursor < 0){
        cursor = 0;
    }
    vgaBuff[cursor] = '\0' | (VGA_COLOR(VGA_BLACK, VGA_BLACK));
}

void VGA_clear(void){
    for(int i = 0; i < width*height; i++){
        VGA_display_char_color('\0', VGA_COLOR(VGA_BLACK, VGA_BLACK));
    }
    cursor = 0;
}
