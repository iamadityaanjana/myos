#include "gdt.h"
#include "idt.h"
#include "memory.h"
#include "paging.h"
#include "framebuffer.h"
#include "compositor.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "io.h"
#include "pfs.h"
#include "shell.h"

#define UI_SHELL_MAX_LINES 64
#define UI_SHELL_LINE_LEN 64
#define UI_INPUT_LEN 63
#define UI_FOLDER_MAX_LINES 12
#define UI_FOLDER_LINE_LEN 32
#define UI_VISIBLE_ROWS 22

static char ui_shell_lines[UI_SHELL_MAX_LINES][UI_SHELL_LINE_LEN];
static int ui_shell_line_count = 0;
static char ui_input[UI_INPUT_LEN];
static int ui_input_len = 0;
static int shell_window_visible = 0;
static int folders_window_visible = 0;
static int shell_window_id = -1;
static int folders_window_id = -1;
static int ui_shell_view_start = 0;
static volatile int ui_dirty = 0;

static int str_equals(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == b[i];
}

static int str_starts_with(const char* text, const char* prefix) {
    int i = 0;
    while (prefix[i]) {
        if (text[i] != prefix[i]) {
            return 0;
        }
        i++;
    }
    return 1;
}

static void copy_limited(char* dst, int max_len, const char* src) {
    int i = 0;
    if (!dst || max_len <= 0) {
        return;
    }
    if (src) {
        while (src[i] && i < (max_len - 1)) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void ui_push_shell_line(const char* text) {
    if (ui_shell_line_count < UI_SHELL_MAX_LINES) {
        copy_limited(ui_shell_lines[ui_shell_line_count], UI_SHELL_LINE_LEN, text);
        ui_shell_line_count++;
    } else {
        for (int i = 1; i < UI_SHELL_MAX_LINES; i++) {
            copy_limited(ui_shell_lines[i - 1], UI_SHELL_LINE_LEN, ui_shell_lines[i]);
        }
        copy_limited(ui_shell_lines[UI_SHELL_MAX_LINES - 1], UI_SHELL_LINE_LEN, text);
    }
}

static void ui_sync_shell_to_compositor() {
    compositor_set_shell_line_count(ui_shell_line_count);
    for (int i = 0; i < ui_shell_line_count; i++) {
        compositor_set_shell_line(i, ui_shell_lines[i]);
    }
    compositor_set_shell_view_start(ui_shell_view_start);
    compositor_set_shell_input(ui_input);
    ui_dirty = 1;
}

static void ui_refresh_folders_from_pfs() {
    pfs_file_info_t list[UI_FOLDER_MAX_LINES];
    char line[UI_FOLDER_LINE_LEN];

    if (!pfs_is_ready()) {
        compositor_set_folders_line_count(1);
        compositor_set_folders_line(0, "PFS NOT READY");
        ui_dirty = 1;
        return;
    }

    int count = pfs_list(list, UI_FOLDER_MAX_LINES);
    compositor_set_folders_item_count(count);

    if (count <= 0) {
        compositor_set_folders_line_count(1);
        compositor_set_folders_line(0, "EMPTY");
        ui_dirty = 1;
        return;
    }

    compositor_set_folders_line_count(count);
    for (int i = 0; i < count; i++) {
        int j = 0;
        const char* name = list[i].name;
        while (name[j] && j < (UI_FOLDER_LINE_LEN - 8)) {
            char c = name[j];
            if (!((c >= 'a' && c <= 'z') ||
                  (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') ||
                  c == '.' || c == '_' || c == '-')) {
                c = '_';
            }
            line[j] = c;
            j++;
        }
        line[j++] = ' ';
        line[j++] = '-';
        line[j++] = ' ';

        int size = (int)list[i].size;
        if (size == 0) {
            line[j++] = '0';
        } else {
            char tmp[8];
            int t = 0;
            while (size > 0 && t < 7) {
                tmp[t++] = (char)('0' + (size % 10));
                size /= 10;
            }
            while (t > 0 && j < (UI_FOLDER_LINE_LEN - 1)) {
                line[j++] = tmp[--t];
            }
        }
        line[j] = '\0';
        compositor_set_folders_line(i, line);
    }
    ui_dirty = 1;
}

static void ui_scroll_to_bottom() {
    int visible_rows = UI_VISIBLE_ROWS;
    int max_top = ui_shell_line_count > visible_rows ? (ui_shell_line_count - visible_rows) : 0;
    ui_shell_view_start = max_top;
}

static void ui_execute_command(const char* cmd) {
    if (!cmd || cmd[0] == '\0') {
        return;
    }

    if (str_equals(cmd, "help")) {
        const char* lines[40];
        int n = shell_get_help_lines(lines, 40);
        for (int i = 0; i < n; i++) {
            ui_push_shell_line(lines[i]);
        }
        ui_push_shell_line("Use Up/Down arrows to scroll output");
        return;
    }

    if (str_equals(cmd, "clear")) {
        ui_shell_line_count = 0;
        return;
    }

    if (str_equals(cmd, "ls")) {
        pfs_file_info_t list[8];
        int count = pfs_is_ready() ? pfs_list(list, 8) : 0;
        if (!pfs_is_ready()) {
            ui_push_shell_line("PFS NOT READY");
            return;
        }
        if (count <= 0) {
            ui_push_shell_line("EMPTY");
            return;
        }
        for (int i = 0; i < count; i++) {
            ui_push_shell_line(list[i].name);
        }
        return;
    }

    if (str_starts_with(cmd, "cat ")) {
        uint8_t data[64];
        uint32_t size = 0;
        if (!pfs_read_file(cmd + 4, data, sizeof(data), &size)) {
            ui_push_shell_line("NOT FOUND");
            return;
        }
        char line[UI_SHELL_LINE_LEN];
        int n = (int)size;
        if (n > UI_SHELL_LINE_LEN - 1) n = UI_SHELL_LINE_LEN - 1;
        for (int i = 0; i < n; i++) {
            char c = (char)data[i];
            if (c < 32 || c > 126) c = '.';
            line[i] = c;
        }
        line[n] = '\0';
        ui_push_shell_line(line);
        return;
    }

    if (str_starts_with(cmd, "hexdump ")) {
        uint8_t data[64];
        uint32_t size = 0;
        if (!pfs_read_file(cmd + 8, data, sizeof(data), &size)) {
            ui_push_shell_line("NOT FOUND");
            return;
        }

        static const char* hex = "0123456789ABCDEF";
        char line[UI_SHELL_LINE_LEN];
        for (uint32_t off = 0; off < size; off += 8) {
            int p = 0;
            for (uint32_t i = 0; i < 8 && (off + i) < size && p < (UI_SHELL_LINE_LEN - 4); i++) {
                uint8_t b = data[off + i];
                line[p++] = hex[(b >> 4) & 0x0F];
                line[p++] = hex[b & 0x0F];
                line[p++] = ' ';
            }
            line[p] = '\0';
            ui_push_shell_line(line);
        }
        return;
    }

    if (str_starts_with(cmd, "touch ")) {
        if (pfs_write_file(cmd + 6, (const uint8_t*)"", 0)) {
            ui_push_shell_line("OK");
        } else {
            ui_push_shell_line("FAILED");
        }
        return;
    }

    if (str_starts_with(cmd, "rm ")) {
        if (pfs_delete_file(cmd + 3)) {
            ui_push_shell_line("DELETED");
        } else {
            ui_push_shell_line("NOT FOUND");
        }
        return;
    }

    if (str_starts_with(cmd, "write ")) {
        const char* p = cmd + 6;
        char name[16];
        int i = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ' && i < 15) {
            name[i++] = *p++;
        }
        name[i] = '\0';
        while (*p == ' ') p++;
        if (name[0] == '\0' || *p == '\0') {
            ui_push_shell_line("USAGE: WRITE FILE TEXT");
            return;
        }
        int len = 0;
        while (p[len] && len < 120) len++;
        if (pfs_write_file(name, (const uint8_t*)p, (uint32_t)len)) {
            ui_push_shell_line("SAVED");
        } else {
            ui_push_shell_line("FAILED");
        }
        return;
    }

    ui_push_shell_line("UNKNOWN");
}

static void ui_keyboard_handler(char c) {
    if (!shell_window_visible) {
        return;
    }

    if (c == '\n') {
        char command[UI_INPUT_LEN];
        copy_limited(command, UI_INPUT_LEN, ui_input);
        if (ui_input_len > 0) {
            ui_push_shell_line(ui_input);
            ui_execute_command(command);
            ui_refresh_folders_from_pfs();
        }
        ui_scroll_to_bottom();
        ui_input_len = 0;
        ui_input[0] = '\0';
        ui_sync_shell_to_compositor();
        return;
    }

    if (c == '\b') {
        if (ui_input_len > 0) {
            ui_input_len--;
            ui_input[ui_input_len] = '\0';
            ui_sync_shell_to_compositor();
        }
        return;
    }

    if (c >= 32 && c <= 126 && ui_input_len < (UI_INPUT_LEN - 1)) {
        ui_input[ui_input_len++] = c;
        ui_input[ui_input_len] = '\0';
        ui_sync_shell_to_compositor();
    }
}

static void ui_scancode_handler(uint8_t scancode, int extended) {
    if (!shell_window_visible || !extended) {
        return;
    }

    int visible_rows = UI_VISIBLE_ROWS;
    int max_top = ui_shell_line_count > visible_rows ? (ui_shell_line_count - visible_rows) : 0;

    if (scancode == 0x48) {
        if (ui_shell_view_start > 0) {
            ui_shell_view_start--;
            ui_sync_shell_to_compositor();
        }
    } else if (scancode == 0x50) {
        if (ui_shell_view_start < max_top) {
            ui_shell_view_start++;
            ui_sync_shell_to_compositor();
        }
    }
}

void kernel_main() {
    outb(0xE9, 'K');

    gdt_init();
    outb(0xE9, 'G');

    memory_init();
    outb(0xE9, 'M');

    paging_init();
    outb(0xE9, 'V');

    pfs_init();

    framebuffer_init();
    outb(0xE9, 'F');

    compositor_init();
    outb(0xE9, 'C');

    idt_init();
    pic_remap();
    timer_init(100);

    keyboard_init();
    keyboard_set_char_handler(ui_keyboard_handler);
    keyboard_set_scancode_handler(ui_scancode_handler);
    mouse_init();
    mouse_set_shell_events_enabled(0);
    mouse_set_bounds(FB_WIDTH - 1, FB_HEIGHT - 1);
    mouse_set_motion_divider(1, 1);
    __asm__ volatile("sti");

    (void)timer_get_ticks();

    int w1_x = 2;
    int w1_y = 2;
    int w1_w = FB_WIDTH - 4;
    int w1_h = FB_HEIGHT - 4;

    int w2_x = 64;
    int w2_y = 78;
    int w2_w = 232;
    int w2_h = 108;

    int w1 = compositor_add_window(w1_x, w1_y, w1_w, w1_h, 3, 11);
    int w2 = compositor_add_window(w2_x, w2_y, w2_w, w2_h, 6, 12);
    compositor_set_window_visible(w1, 0);
    compositor_set_window_visible(w2, 0);
    shell_window_id = w1;
    folders_window_id = w2;

    ui_push_shell_line("TYPE HELP");
    ui_sync_shell_to_compositor();
    ui_refresh_folders_from_pfs();

    int cursor_x = 40;
    int cursor_y = 32;
    int prev_x = -1;
    int prev_y = -1;
    int left_down = 0;
    int right_down = 0;
    int left_pressed = 0;
    int left_released = 0;
    int right_pressed = 0;
    int right_released = 0;

    int drag_target = 0;
    int drag_off_x = 0;
    int drag_off_y = 0;

    uint32_t tick = 0;

    compositor_render(cursor_x, cursor_y, tick);

    while (1) {
        __asm__ volatile("hlt");

        keyboard_process_pending();
        mouse_process_pending();

        mouse_get_position(&cursor_x, &cursor_y);
        mouse_get_buttons(&left_down,
                          &right_down,
                          &left_pressed,
                          &left_released,
                          &right_pressed,
                          &right_released);

        int needs_redraw = 0;

        if (cursor_x != prev_x || cursor_y != prev_y) {
            prev_x = cursor_x;
            prev_y = cursor_y;
            needs_redraw = 1;
        }

        if (left_pressed) {
            compositor_icon_t icon = compositor_desktop_hit_icon(cursor_x, cursor_y);
            if (icon == COMPOSITOR_ICON_SHELL) {
                compositor_set_window_visible(w1, 1);
                compositor_set_window_position(w1, w1_x, w1_y);
                shell_window_visible = 1;
                ui_scroll_to_bottom();
                ui_sync_shell_to_compositor();
            } else if (icon == COMPOSITOR_ICON_FOLDERS) {
                ui_refresh_folders_from_pfs();
                compositor_set_window_visible(w2, 1);
                compositor_set_window_position(w2, w2_x, w2_y);
                folders_window_visible = 1;
            }

            if (compositor_window_hit_close(w2, cursor_x, cursor_y)) {
                compositor_set_window_visible(w2, 0);
                folders_window_visible = 0;
            } else if (compositor_window_hit_close(w1, cursor_x, cursor_y)) {
                compositor_set_window_visible(w1, 0);
                shell_window_visible = 0;
            } else if (compositor_window_hit_titlebar(w2, cursor_x, cursor_y)) {
                drag_target = 2;
                drag_off_x = cursor_x - w2_x;
                drag_off_y = cursor_y - w2_y;
            } else if (compositor_window_hit_titlebar(w1, cursor_x, cursor_y)) {
                drag_target = 1;
                drag_off_x = cursor_x - w1_x;
                drag_off_y = cursor_y - w1_y;
            }
            needs_redraw = 1;
        }

        if (left_down && drag_target == 1 && compositor_window_is_visible(w1)) {
            w1_x = cursor_x - drag_off_x;
            w1_y = cursor_y - drag_off_y;
            compositor_set_window_position(w1, w1_x, w1_y);
            needs_redraw = 1;
        } else if (left_down && drag_target == 2 && compositor_window_is_visible(w2)) {
            w2_x = cursor_x - drag_off_x;
            w2_y = cursor_y - drag_off_y;
            compositor_set_window_position(w2, w2_x, w2_y);
            needs_redraw = 1;
        }

        if (left_released) {
            drag_target = 0;
            needs_redraw = 1;
        }

        if (right_pressed || right_released) {
            needs_redraw = 1;
        }

        if (needs_redraw) {
            compositor_render(cursor_x, cursor_y, tick);
            ui_dirty = 0;
        } else if (ui_dirty) {
            compositor_render(cursor_x, cursor_y, tick);
            ui_dirty = 0;
        }
    }
}
