#include "screen.h"
#include "io.h"

static uint16_t* video_memory = (uint16_t*) 0xB8000;
static uint16_t text_buffer[SCROLLBACK_LINES][VGA_WIDTH];

static int cursor_row = 0;
static int cursor_col = 0;
static int total_lines = 1;
static int view_top_line = 0;
static uint8_t current_color = 0x07;

static uint16_t make_cell(char c) {
    return (uint16_t)((current_color << 8) | (uint8_t)c);
}

static void render_view() {
    for (int row = 0; row < VGA_HEIGHT; row++) {
        int source_row = view_top_line + row;
        for (int col = 0; col < VGA_WIDTH; col++) {
            if (source_row < total_lines) {
                video_memory[row * VGA_WIDTH + col] = text_buffer[source_row][col];
            } else {
                video_memory[row * VGA_WIDTH + col] = make_cell(' ');
            }
        }
    }
}

static void shift_buffer_up_one_line() {
    for (int row = 1; row < SCROLLBACK_LINES; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            text_buffer[row - 1][col] = text_buffer[row][col];
        }
    }

    for (int col = 0; col < VGA_WIDTH; col++) {
        text_buffer[SCROLLBACK_LINES - 1][col] = make_cell(' ');
    }

    if (cursor_row > 0) {
        cursor_row--;
    }
    if (total_lines > 1) {
        total_lines--;
    }
    if (view_top_line > 0) {
        view_top_line--;
    }
}

static void ensure_cursor_row_available() {
    while (cursor_row >= SCROLLBACK_LINES) {
        shift_buffer_up_one_line();
    }

    if (cursor_row >= total_lines) {
        total_lines = cursor_row + 1;
    }

    if (total_lines > SCROLLBACK_LINES) {
        total_lines = SCROLLBACK_LINES;
    }
}

static int is_view_at_bottom() {
    return view_top_line + VGA_HEIGHT >= total_lines;
}

void update_cursor() {
    int visible_row = cursor_row - view_top_line;
    uint16_t pos;

    if (visible_row < 0 || visible_row >= VGA_HEIGHT) {
        pos = (uint16_t)((VGA_HEIGHT - 1) * VGA_WIDTH);
    } else {
        pos = (uint16_t)(visible_row * VGA_WIDTH + cursor_col);
    }

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen() {
    for (int row = 0; row < SCROLLBACK_LINES; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            text_buffer[row][col] = make_cell(' ');
        }
    }

    cursor_row = 0;
    cursor_col = 0;
    total_lines = 1;
    view_top_line = 0;
    render_view();
    update_cursor();
}

void put_char(char c) {
    int was_at_bottom = is_view_at_bottom();

    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        } else if (cursor_row > 0) {
            cursor_row--;
            cursor_col = VGA_WIDTH - 1;
        }

        text_buffer[cursor_row][cursor_col] = make_cell(' ');
        render_view();
        update_cursor();
        return;
    }

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        ensure_cursor_row_available();
        if (was_at_bottom) {
            view_top_line = total_lines > VGA_HEIGHT ? total_lines - VGA_HEIGHT : 0;
        }
        render_view();
        update_cursor();
        return;
    }

    text_buffer[cursor_row][cursor_col] = make_cell(c);

    cursor_col++;

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
        ensure_cursor_row_available();
    }

    if (was_at_bottom) {
        view_top_line = total_lines > VGA_HEIGHT ? total_lines - VGA_HEIGHT : 0;
    }

    render_view();
    update_cursor();
}

void print(const char* str) {
    int i = 0;
    while (str[i]) {
        put_char(str[i]);
        i++;
    }
}

void set_color(uint8_t color) {
    current_color = color;
}

void scroll_view_up(int lines) {
    if (lines <= 0) {
        return;
    }

    if (view_top_line >= lines) {
        view_top_line -= lines;
    } else {
        view_top_line = 0;
    }

    render_view();
    update_cursor();
}

void scroll_view_down(int lines) {
    if (lines <= 0) {
        return;
    }

    int max_top = total_lines > VGA_HEIGHT ? total_lines - VGA_HEIGHT : 0;
    view_top_line += lines;
    if (view_top_line > max_top) {
        view_top_line = max_top;
    }

    render_view();
    update_cursor();
}
