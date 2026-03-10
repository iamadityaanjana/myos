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
void set_cursor_enabled(int enabled);
void scroll_view_up(int lines);
void scroll_view_down(int lines);
void draw_char_at(int row, int col, char c, uint8_t color);
void draw_text_at(int row, int col, const char* text, uint8_t color);
void fill_rect(int row, int col, int height, int width, char c, uint8_t color);
void screen_begin_batch();
void screen_end_batch();

#endif
