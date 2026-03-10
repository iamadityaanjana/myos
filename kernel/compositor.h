#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "types.h"

#define COMPOSITOR_MAX_WINDOWS 8

typedef struct compositor_window {
    int x;
    int y;
    int w;
    int h;
    uint8_t body_color;
    uint8_t title_color;
    int visible;
} compositor_window_t;

typedef enum {
    COMPOSITOR_ICON_NONE = 0,
    COMPOSITOR_ICON_SHELL = 1,
    COMPOSITOR_ICON_FOLDERS = 2
} compositor_icon_t;

void compositor_init();
int compositor_add_window(int x, int y, int w, int h, uint8_t body_color, uint8_t title_color);
void compositor_set_window_position(int id, int x, int y);
void compositor_set_window_visible(int id, int visible);
int compositor_window_is_visible(int id);
void compositor_set_folders_item_count(int count);
void compositor_set_shell_line(int index, const char* text);
void compositor_set_shell_line_count(int count);
void compositor_set_shell_input(const char* text);
void compositor_set_shell_view_start(int start);
void compositor_set_folders_line(int index, const char* text);
void compositor_set_folders_line_count(int count);
int compositor_window_hit_titlebar(int id, int x, int y);
int compositor_window_hit_close(int id, int x, int y);
compositor_icon_t compositor_desktop_hit_icon(int x, int y);
void compositor_render(int cursor_x, int cursor_y, uint32_t tick);

#endif
