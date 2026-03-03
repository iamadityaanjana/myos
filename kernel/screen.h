#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define SCROLLBACK_LINES 512

void clear_screen();
void put_char(char c);
void print(const char* str);
void set_color(uint8_t color);
void update_cursor();
void scroll_view_up(int lines);
void scroll_view_down(int lines);

#endif
