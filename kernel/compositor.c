#include "compositor.h"

#include "framebuffer.h"

static compositor_window_t windows[COMPOSITOR_MAX_WINDOWS];
static int window_count = 0;
static int folders_item_count = 0;
static char shell_lines[64][64];
static int shell_line_count = 0;
static int shell_view_start = 0;
static char shell_input[64];
static char folder_lines[12][32];
static int folder_line_count = 0;

#define ICON_W 28
#define ICON_H 28
#define ICON_SHELL_X 20
#define ICON_SHELL_Y 24
#define ICON_FOLDERS_X 20
#define ICON_FOLDERS_Y 70

static int in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && y >= ry && x < (rx + rw) && y < (ry + rh);
}

static void copy_limited(char* dst, int dst_len, const char* src) {
    if (!dst || dst_len <= 0) {
        return;
    }

    int i = 0;
    if (src) {
        while (src[i] && i < (dst_len - 1)) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static uint8_t glyph_row(char ch, int row) {
    switch (ch) {
        case 'A': return (uint8_t[]){14,17,17,31,17}[row];
        case 'B': return (uint8_t[]){30,17,30,17,30}[row];
        case 'C': return (uint8_t[]){15,16,16,16,15}[row];
        case 'D': return (uint8_t[]){30,17,17,17,30}[row];
        case 'E': return (uint8_t[]){31,16,30,16,31}[row];
        case 'F': return (uint8_t[]){31,16,30,16,16}[row];
        case 'G': return (uint8_t[]){15,16,23,17,15}[row];
        case 'H': return (uint8_t[]){17,17,31,17,17}[row];
        case 'I': return (uint8_t[]){31,4,4,4,31}[row];
        case 'J': return (uint8_t[]){7,2,2,18,12}[row];
        case 'K': return (uint8_t[]){17,18,28,18,17}[row];
        case 'L': return (uint8_t[]){16,16,16,16,31}[row];
        case 'M': return (uint8_t[]){17,27,21,17,17}[row];
        case 'N': return (uint8_t[]){17,25,21,19,17}[row];
        case 'O': return (uint8_t[]){14,17,17,17,14}[row];
        case 'P': return (uint8_t[]){30,17,30,16,16}[row];
        case 'Q': return (uint8_t[]){14,17,17,19,15}[row];
        case 'R': return (uint8_t[]){30,17,30,18,17}[row];
        case 'S': return (uint8_t[]){15,16,14,1,30}[row];
        case 'T': return (uint8_t[]){31,4,4,4,4}[row];
        case 'U': return (uint8_t[]){17,17,17,17,14}[row];
        case 'V': return (uint8_t[]){17,17,17,10,4}[row];
        case 'W': return (uint8_t[]){17,17,21,27,17}[row];
        case 'X': return (uint8_t[]){17,10,4,10,17}[row];
        case 'Y': return (uint8_t[]){17,10,4,4,4}[row];
        case 'Z': return (uint8_t[]){31,2,4,8,31}[row];
        case '0': return (uint8_t[]){14,19,21,25,14}[row];
        case '1': return (uint8_t[]){4,12,4,4,14}[row];
        case '2': return (uint8_t[]){14,1,14,16,31}[row];
        case '3': return (uint8_t[]){30,1,14,1,30}[row];
        case '4': return (uint8_t[]){18,18,31,2,2}[row];
        case '5': return (uint8_t[]){31,16,30,1,30}[row];
        case '6': return (uint8_t[]){15,16,30,17,14}[row];
        case '7': return (uint8_t[]){31,2,4,8,8}[row];
        case '8': return (uint8_t[]){14,17,14,17,14}[row];
        case '9': return (uint8_t[]){14,17,15,1,30}[row];
        case '.': return (uint8_t[]){0,0,0,0,4}[row];
        case '-': return (uint8_t[]){0,0,14,0,0}[row];
        case '_': return (uint8_t[]){0,0,0,0,31}[row];
        case '/': return (uint8_t[]){1,2,4,8,16}[row];
        case ':': return (uint8_t[]){0,4,0,4,0}[row];
        case '>': return (uint8_t[]){8,4,2,4,8}[row];
        case ' ': return 0;
        default: return (uint8_t[]){31,1,14,0,4}[row];
    }
}

static void draw_char5x5(int x, int y, char ch, uint8_t color) {
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }

    for (int row = 0; row < 5; row++) {
        uint8_t bits = glyph_row(ch, row);
        for (int col = 0; col < 5; col++) {
            if (bits & (1 << (4 - col))) {
                framebuffer_put_pixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_text5x5(int x, int y, const char* text, uint8_t color) {
    if (!text) {
        return;
    }

    int i = 0;
    while (text[i]) {
        draw_char5x5(x + i * 6, y, text[i], color);
        i++;
    }
}

static void draw_icon_shell() {
    framebuffer_fill_rect(ICON_SHELL_X, ICON_SHELL_Y, ICON_W, ICON_H, 1);
    framebuffer_fill_rect(ICON_SHELL_X + 2, ICON_SHELL_Y + 2, ICON_W - 4, ICON_H - 4, 0);
    framebuffer_fill_rect(ICON_SHELL_X + 6, ICON_SHELL_Y + 8, 14, 2, 10);
    framebuffer_fill_rect(ICON_SHELL_X + 6, ICON_SHELL_Y + 13, 10, 2, 10);
}

static void draw_icon_folders() {
    framebuffer_fill_rect(ICON_FOLDERS_X + 3, ICON_FOLDERS_Y + 7, ICON_W - 6, ICON_H - 8, 14);
    framebuffer_fill_rect(ICON_FOLDERS_X + 5, ICON_FOLDERS_Y + 4, 10, 5, 14);
    framebuffer_fill_rect(ICON_FOLDERS_X + 5, ICON_FOLDERS_Y + 11, ICON_W - 10, ICON_H - 14, 6);
}

void compositor_init() {
    window_count = 0;
    folders_item_count = 0;
    for (int i = 0; i < COMPOSITOR_MAX_WINDOWS; i++) {
        windows[i].visible = 0;
    }
}

int compositor_add_window(int x, int y, int w, int h, uint8_t body_color, uint8_t title_color) {
    if (window_count >= COMPOSITOR_MAX_WINDOWS || w < 16 || h < 16) {
        return -1;
    }

    windows[window_count].x = x;
    windows[window_count].y = y;
    windows[window_count].w = w;
    windows[window_count].h = h;
    windows[window_count].body_color = body_color;
    windows[window_count].title_color = title_color;
    windows[window_count].visible = 1;

    window_count++;
    return window_count - 1;
}

void compositor_set_window_position(int id, int x, int y) {
    if (id < 0 || id >= window_count) {
        return;
    }

    windows[id].x = x;
    windows[id].y = y;
}

void compositor_set_window_visible(int id, int visible) {
    if (id < 0 || id >= window_count) {
        return;
    }
    windows[id].visible = visible ? 1 : 0;
}

int compositor_window_is_visible(int id) {
    if (id < 0 || id >= window_count) {
        return 0;
    }
    return windows[id].visible;
}

void compositor_set_folders_item_count(int count) {
    if (count < 0) {
        count = 0;
    }
    if (count > 24) {
        count = 24;
    }
    folders_item_count = count;
}

void compositor_set_shell_line(int index, const char* text) {
    if (index < 0 || index >= 64) {
        return;
    }
    copy_limited(shell_lines[index], 64, text);
}

void compositor_set_shell_line_count(int count) {
    if (count < 0) count = 0;
    if (count > 64) count = 64;
    shell_line_count = count;
}

void compositor_set_shell_input(const char* text) {
    copy_limited(shell_input, 64, text);
}

void compositor_set_shell_view_start(int start) {
    if (start < 0) {
        start = 0;
    }
    shell_view_start = start;
}

void compositor_set_folders_line(int index, const char* text) {
    if (index < 0 || index >= 12) {
        return;
    }
    copy_limited(folder_lines[index], 32, text);
}

void compositor_set_folders_line_count(int count) {
    if (count < 0) count = 0;
    if (count > 12) count = 12;
    folder_line_count = count;
}

int compositor_window_hit_titlebar(int id, int x, int y) {
    if (id < 0 || id >= window_count || !windows[id].visible) {
        return 0;
    }
    return in_rect(x, y, windows[id].x, windows[id].y, windows[id].w, 12);
}

int compositor_window_hit_close(int id, int x, int y) {
    if (id < 0 || id >= window_count || !windows[id].visible) {
        return 0;
    }
    return in_rect(x, y, windows[id].x + windows[id].w - 12, windows[id].y, 12, 12);
}

compositor_icon_t compositor_desktop_hit_icon(int x, int y) {
    if (in_rect(x, y, ICON_SHELL_X, ICON_SHELL_Y, ICON_W, ICON_H)) {
        return COMPOSITOR_ICON_SHELL;
    }
    if (in_rect(x, y, ICON_FOLDERS_X, ICON_FOLDERS_Y, ICON_W, ICON_H)) {
        return COMPOSITOR_ICON_FOLDERS;
    }
    return COMPOSITOR_ICON_NONE;
}

static void draw_window(const compositor_window_t* w) {
    if (!w || !w->visible) {
        return;
    }

    // Simple drop shadow to separate overlapping windows.
    framebuffer_fill_rect(w->x + 3, w->y + 3, w->w, w->h, 8);

    framebuffer_fill_rect(w->x, w->y, w->w, w->h, w->body_color);
    framebuffer_fill_rect(w->x, w->y, w->w, 12, w->title_color);
    framebuffer_fill_rect(w->x + w->w - 10, w->y + 3, 7, 2, 12);
    framebuffer_fill_rect(w->x + w->w - 10, w->y + 7, 7, 2, 12);

    // Border.
    framebuffer_fill_rect(w->x, w->y, w->w, 1, 15);
    framebuffer_fill_rect(w->x, w->y + w->h - 1, w->w, 1, 15);
    framebuffer_fill_rect(w->x, w->y, 1, w->h, 15);
    framebuffer_fill_rect(w->x + w->w - 1, w->y, 1, w->h, 15);
}

static void draw_shell_window_contents(const compositor_window_t* w) {
    if (!w || !w->visible) {
        return;
    }

    int base_x = w->x + 8;
    int base_y = w->y + 16;

    draw_text5x5(base_x, base_y, "SHELL", 15);

    int max_rows = 22;
    for (int row = 0; row < max_rows; row++) {
        int idx = shell_view_start + row;
        if (idx >= shell_line_count) {
            break;
        }
        draw_text5x5(base_x, base_y + 10 + row * 7, shell_lines[idx], 15);
    }

    draw_text5x5(base_x, w->y + w->h - 10, ">", 14);
    draw_text5x5(base_x + 8, w->y + w->h - 10, shell_input, 14);
}

static void draw_folders_window_contents(const compositor_window_t* w) {
    if (!w || !w->visible) {
        return;
    }

    int base_x = w->x + 8;
    int base_y = w->y + 16;

    draw_text5x5(base_x, base_y, "FOLDERS", 15);

    int rows = folder_line_count;
    if (rows == 0) {
        if (folders_item_count == 0) {
            draw_text5x5(base_x, base_y + 10, "EMPTY", 15);
        }
        return;
    }

    for (int i = 0; i < rows && i < 10; i++) {
        draw_text5x5(base_x, base_y + 10 + i * 7, folder_lines[i], 15);
    }
}

static void draw_cursor(int x, int y) {
    // 8x8 arrow cursor.
    for (int i = 0; i < 8; i++) {
        framebuffer_put_pixel(x, y + i, 15);
    }

    for (int i = 0; i < 6; i++) {
        framebuffer_put_pixel(x + i, y + i, 15);
    }

    framebuffer_put_pixel(x + 2, y + 6, 15);
    framebuffer_put_pixel(x + 3, y + 6, 15);
    framebuffer_put_pixel(x + 3, y + 7, 15);
    framebuffer_put_pixel(x + 4, y + 7, 15);
}

void compositor_render(int cursor_x, int cursor_y, uint32_t tick) {
    (void)tick;

    // Stable solid background for readable desktop visuals.
    framebuffer_fill_rect(0, 0, FB_WIDTH, FB_HEIGHT, 11);

    draw_icon_shell();
    draw_icon_folders();

    for (int i = 0; i < window_count; i++) {
        draw_window(&windows[i]);
    }

    if (window_count > 0) {
        draw_shell_window_contents(&windows[0]);
    }
    if (window_count > 1) {
        draw_folders_window_contents(&windows[1]);
    }

    draw_cursor(cursor_x, cursor_y);
    framebuffer_present();
}
