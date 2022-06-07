#define VGA_BLACK 0x0
#define VGA_BLUE 0x1
#define VGA_GREEN 0x2
#define VGA_CYAN 0x3
#define VGA_RED 0x4
#define VGA_MAGENTA 0x5
#define VGA_BROWN 0x6
#define VGA_LIGHT_GRAY 0x7
#define VGA_DARK_GRAY 0x8
#define VGA_LIGHT_BLUE 0x9
#define VGA_LIGHT_GREEN 0xA
#define VGA_LIGHT_CYAN 0xB
#define VGA_LIGHT_RED 0xC
#define VGA_LIGHT_MAGENTA 0xD
#define VGA_YELLOW 0xE
#define VGA_WHITE 0xF

#define VGA_COLOR(fg, bg) (fg) | (bg) << 4

void VGA_display_attr_char(int x, int y, char c, int fg, int bg);
void VGA_display_char_at(unsigned short row, unsigned short column, char c, char color);
void VGA_display_char(char);
void VGA_remove_char(void);
void VGA_display_str(const char *);
void VGA_clear(void);
int VGA_row_count(void);
int VGA_col_count(void);