#include "shell.h"

#include "screen.h"
#include "io.h"
#include "printk.h"
#include "memory.h"
#include "paging.h"
#include "rtc.h"
#include "timer.h"
#include "scheduler.h"
#include "ramfs.h"
#include "pfs.h"

#define SHELL_MAX_INPUT 64

static char input_buffer[SHELL_MAX_INPUT];
static int input_len = 0;

static int desktop_mode = 0;
static int desktop_mouse_x = 4;
static int desktop_mouse_y = 4;
static uint32_t last_left_click_tick = 0;
static uint32_t last_desktop_redraw_tick = 0;

static int fs_window_open = 0;
static int fs_window_x = 25;
static int fs_window_y = 6;
static int fs_window_w = 36;
static int fs_window_h = 12;
static int fs_drag_active = 0;
static int fs_drag_off_x = 0;
static int fs_drag_off_y = 0;
static int fs_resize_active = 0;

static int context_visible = 0;
static int context_x = 0;
static int context_y = 0;
static const char* context_target = 0;

static const char* g_shell_help_lines[] = {
    "Commands:",
    "  help      - list commands",
    "  clear     - clear screen",
    "  about     - about MyOS",
    "  shutdown  - power off emulator",
    "  reboot    - reboot CPU",
    "  echo TXT  - print text",
    "  time      - show current time/date",
    "  set time HH:MM:SS - override displayed time",
    "  calendar  - show current month calendar",
    "  heap      - show heap usage",
    "  kmalloc N - allocate N bytes",
    "  memmap    - show kernel memory map",
    "  paging    - show paging status",
    "  ticks     - show timer ticks",
    "  uptime    - show uptime in seconds",
    "  ls        - list files",
    "  cat FILE  - print file contents",
    "  hexdump FILE - hex view of a file",
    "  touch FILE - create empty file",
    "  write FILE TEXT - save text to file",
    "  rm FILE   - delete file",
    "  rls       - list RAMFS demo files",
    "  desktop   - open desktop mode",
    "  bg run TASK|all - start background tasks",
    "  bg list   - list running background tasks",
    "  bg stop ID|TASK|all - stop background tasks"
};

static void desktop_redraw();

static void print_prompt() {
    set_color(0x0E);
    print("MyOS> ");
    set_color(0x07);
}

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

static int parse_uint(const char* text, int* out_value) {
    int value = 0;
    int i = 0;

    if (!text || text[0] == '\0') {
        return 0;
    }

    while (text[i]) {
        char c = text[i];
        if (c < '0' || c > '9') {
            return 0;
        }
        value = value * 10 + (c - '0');
        i++;
    }

    *out_value = value;
    return 1;
}

static int parse_bg_name(const char* text, char* out_name) {
    if (!text || !out_name) {
        return 0;
    }

    while (*text == ' ') {
        text++;
    }

    int i = 0;
    while (text[i] && text[i] != ' ' && i < (SCHED_TASK_NAME_MAX - 1)) {
        out_name[i] = text[i];
        i++;
    }
    out_name[i] = '\0';

    return i > 0;
}

static int parse_token(const char* text, char* out_token, int token_max) {
    if (!text || !out_token || token_max <= 1) {
        return 0;
    }

    while (*text == ' ') {
        text++;
    }

    int i = 0;
    while (text[i] && text[i] != ' ' && i < (token_max - 1)) {
        out_token[i] = text[i];
        i++;
    }
    out_token[i] = '\0';

    return i > 0;
}

static int parse_two_tokens_and_rest(const char* text, char* token1, int token1_max, const char** out_rest) {
    if (!text || !token1 || token1_max <= 1 || !out_rest) {
        return 0;
    }

    while (*text == ' ') {
        text++;
    }

    int i = 0;
    while (text[i] && text[i] != ' ' && i < (token1_max - 1)) {
        token1[i] = text[i];
        i++;
    }
    token1[i] = '\0';

    if (i == 0) {
        return 0;
    }

    text += i;
    while (*text == ' ') {
        text++;
    }

    *out_rest = text;
    return 1;
}

static void draw_box(int x, int y, int w, int h, uint8_t border_color, uint8_t fill_color) {
    if (w < 2 || h < 2) {
        return;
    }

    fill_rect(y, x, h, w, ' ', fill_color);

    for (int i = 0; i < w; i++) {
        draw_char_at(y, x + i, '-', border_color);
        draw_char_at(y + h - 1, x + i, '-', border_color);
    }
    for (int i = 0; i < h; i++) {
        draw_char_at(y + i, x, '|', border_color);
        draw_char_at(y + i, x + w - 1, '|', border_color);
    }

    draw_char_at(y, x, '+', border_color);
    draw_char_at(y, x + w - 1, '+', border_color);
    draw_char_at(y + h - 1, x, '+', border_color);
    draw_char_at(y + h - 1, x + w - 1, '+', border_color);
}

static int in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x < (rx + rw) && y >= ry && y < (ry + rh);
}

static void desktop_draw_icons() {
    draw_box(4, 3, 14, 6, 0x1F, 0x17);
    draw_text_at(5, 8, "S", 0x1E);
    draw_text_at(7, 6, "Shell", 0x1F);

    draw_box(22, 3, 14, 6, 0x1F, 0x17);
    draw_text_at(5, 26, "F", 0x1E);
    draw_text_at(7, 24, "Files", 0x1F);
}

static void desktop_draw_fs_window() {
    if (!fs_window_open) {
        return;
    }

    if (fs_window_x < 1) fs_window_x = 1;
    if (fs_window_y < 1) fs_window_y = 1;
    if (fs_window_x + fs_window_w > 79) fs_window_x = 79 - fs_window_w;
    if (fs_window_y + fs_window_h > 24) fs_window_y = 24 - fs_window_h;

    draw_box(fs_window_x, fs_window_y, fs_window_w, fs_window_h, 0x0F, 0x70);
    fill_rect(fs_window_y, fs_window_x + 1, 1, fs_window_w - 2, ' ', 0x1F);
    draw_text_at(fs_window_y, fs_window_x + 2, "Filesystem", 0x1F);
    draw_text_at(fs_window_y, fs_window_x + fs_window_w - 6, "[X]", 0x4F);
    draw_char_at(fs_window_y + fs_window_h - 1, fs_window_x + fs_window_w - 2, '#', 0x4F);

    pfs_file_info_t list[8];
    int count = pfs_list(list, 8);

    if (!pfs_is_ready()) {
        draw_text_at(fs_window_y + 2, fs_window_x + 2, "PFS not ready", 0x70);
        return;
    }

    if (count == 0) {
        draw_text_at(fs_window_y + 2, fs_window_x + 2, "No files", 0x70);
        return;
    }

    for (int i = 0; i < count && i < (fs_window_h - 3); i++) {
        draw_text_at(fs_window_y + 2 + i, fs_window_x + 2, list[i].name, 0x70);
    }
}

static void desktop_draw_context_menu() {
    if (!context_visible) {
        return;
    }

    int w = 22;
    int h = 5;
    int x = context_x;
    int y = context_y;

    if (x + w > 79) x = 79 - w;
    if (y + h > 24) y = 24 - h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    draw_box(x, y, w, h, 0x0F, 0x71);
    draw_text_at(y + 1, x + 2, "Right click menu", 0x1F);
    if (context_target) {
        draw_text_at(y + 2, x + 2, context_target, 0x71);
    }
    draw_text_at(y + 3, x + 2, "Double-click to open", 0x71);
}

static void desktop_redraw() {
    screen_begin_batch();

    fill_rect(0, 0, 25, 80, ' ', 0x17);

    fill_rect(0, 0, 1, 80, ' ', 0x1F);
    draw_text_at(0, 2, "MyOS Desktop - Double click icons, right click for options, ESC to exit", 0x1F);

    desktop_draw_icons();
    desktop_draw_fs_window();
    desktop_draw_context_menu();

    draw_char_at(desktop_mouse_y, desktop_mouse_x, '@', 0x4F);

    screen_end_batch();
    last_desktop_redraw_tick = timer_get_ticks();
}

static void enter_desktop_mode() {
    desktop_mode = 1;
    context_visible = 0;
    fs_drag_active = 0;
    fs_resize_active = 0;
    set_cursor_enabled(0);
    desktop_redraw();
}

static void exit_desktop_mode() {
    desktop_mode = 0;
    set_cursor_enabled(1);
    clear_screen();
    print("Exited desktop mode\n");
    print_prompt();
}

static void print_hex_byte(uint8_t b) {
    static const char* hex = "0123456789ABCDEF";
    put_char(hex[(b >> 4) & 0x0F]);
    put_char(hex[b & 0x0F]);
}

static void print_two_digits(int value) {
    put_char((char)('0' + (value / 10) % 10));
    put_char((char)('0' + (value % 10)));
}

static int is_leap_year(int year) {
    if (year % 400 == 0) {
        return 1;
    }
    if (year % 100 == 0) {
        return 0;
    }
    return (year % 4) == 0;
}

static int days_in_month(int month, int year) {
    static const int days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month == 2 && is_leap_year(year)) {
        return 29;
    }

    if (month < 1 || month > 12) {
        return 30;
    }

    return days[month - 1];
}

static int day_of_week(int day, int month, int year) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }

    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) + (5 * j)) % 7;
    return (h + 6) % 7;
}

static void print_time_now() {
    clock_time_t clock_now;
    timer_get_clock(&clock_now);

    rtc_datetime_t now;
    rtc_read_datetime(&now);

    print("Time: ");
    print_two_digits(clock_now.hour);
    put_char(':');
    print_two_digits(clock_now.minute);
    put_char(':');
    print_two_digits(clock_now.second);
    print("  Date: ");
    print_two_digits(now.day);
    put_char('/');
    print_two_digits(now.month);
    put_char('/');
    print_int(now.year);
    put_char('\n');
}

static int parse_set_time(const char* text) {
    int h = 0;
    int m = 0;
    int s = 0;

    if (!text) {
        return 0;
    }

    if (!(text[0] >= '0' && text[0] <= '9' &&
          text[1] >= '0' && text[1] <= '9' &&
          text[2] == ':' &&
          text[3] >= '0' && text[3] <= '9' &&
          text[4] >= '0' && text[4] <= '9' &&
          text[5] == ':' &&
          text[6] >= '0' && text[6] <= '9' &&
          text[7] >= '0' && text[7] <= '9' &&
          text[8] == '\0')) {
        return 0;
    }

    h = (text[0] - '0') * 10 + (text[1] - '0');
    m = (text[3] - '0') * 10 + (text[4] - '0');
    s = (text[6] - '0') * 10 + (text[7] - '0');

    if (h > 23 || m > 59 || s > 59) {
        return 0;
    }

    timer_set_clock((uint8_t)h, (uint8_t)m, (uint8_t)s);
    return 1;
}

static void print_calendar() {
    static const char* month_names[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    rtc_datetime_t now;
    rtc_read_datetime(&now);

    int month = now.month;
    int year = now.year;
    int first_weekday = day_of_week(1, month, year);
    int dim = days_in_month(month, year);

    print(month_names[month - 1]);
    print(" ");
    print_int(year);
    put_char('\n');
    print("Su Mo Tu We Th Fr Sa\n");

    for (int i = 0; i < first_weekday; i++) {
        print("   ");
    }

    for (int day = 1; day <= dim; day++) {
        if (day < 10) {
            put_char(' ');
        }
        print_int(day);

        if ((first_weekday + day) % 7 == 0) {
            put_char('\n');
        } else {
            put_char(' ');
        }
    }

    put_char('\n');
}

static void print_heap_stats() {
    print("Heap start: ");
    print_hex(kheap_start());
    put_char('\n');

    print("Heap end:   ");
    print_hex(kheap_end());
    put_char('\n');

    print("Heap used:  ");
    print_int((int)kheap_used());
    print(" bytes\n");

    print("Heap free:  ");
    print_int((int)kheap_free());
    print(" bytes\n");
}

static void print_memory_map() {
    memory_region_t regions[8];
    int count = memory_get_map(regions, 8);

    print("Memory map:\n");
    for (int i = 0; i < count; i++) {
        print("  ");
        print(regions[i].name);
        print(": ");
        print_hex(regions[i].start);
        print(" - ");
        print_hex(regions[i].end - 1);
        put_char('\n');
    }
}

static void print_scheduler_tasks() {
    sched_task_info_t list[SCHED_MAX_TASKS];
    int n = scheduler_list_tasks(list, SCHED_MAX_TASKS);

    if (n == 0) {
        print("No background tasks running.\n");
        return;
    }

    print("Background tasks:\n");
    for (int i = 0; i < n; i++) {
        print("  #");
        print_int(list[i].id);
        print(" ");
        print(list[i].name);
        print(" runs=");
        print_int((int)list[i].runs);
        print(" count=");
        print_int((int)list[i].counter);
        print(" last_tick=");
        print_int((int)list[i].last_tick);
        put_char('\n');
    }
}

static void print_fs_list() {
    pfs_file_info_t list[32];
    int count = pfs_list(list, 32);

    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (count == 0) {
        print("PFS is empty\n");
        return;
    }

    print("PFS files:\n");
    for (int i = 0; i < count; i++) {
        print("  ");
        print(list[i].name);
        print("  ");
        print_int((int)list[i].size);
        print(" bytes\n");
    }
}

static void cat_fs_file(const char* name) {
    uint8_t data[512];
    uint32_t size = 0;

    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (!pfs_read_file(name, data, sizeof(data), &size)) {
        print("cat: file not found\n");
        return;
    }

    for (uint32_t i = 0; i < size; i++) {
        char c = (char)data[i];
        if (c == '\n' || c == '\t' || (c >= 32 && c <= 126)) {
            put_char(c);
        } else {
            put_char('.');
        }
    }

    if (size == 0 || data[size - 1] != '\n') {
        put_char('\n');
    }
}

static void hexdump_fs_file(const char* name) {
    uint8_t data[512];
    uint32_t size = 0;

    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (!pfs_read_file(name, data, sizeof(data), &size)) {
        print("hexdump: file not found\n");
        return;
    }

    for (uint32_t offset = 0; offset < size; offset += 16) {
        print_hex(offset);
        print(": ");

        for (uint32_t i = 0; i < 16; i++) {
            uint32_t idx = offset + i;
            if (idx < size) {
                print_hex_byte(data[idx]);
            } else {
                print("  ");
            }
            put_char(' ');
        }

        put_char('|');
        for (uint32_t i = 0; i < 16; i++) {
            uint32_t idx = offset + i;
            if (idx < size) {
                char c = (char)data[idx];
                if (c >= 32 && c <= 126) {
                    put_char(c);
                } else {
                    put_char('.');
                }
            } else {
                put_char(' ');
            }
        }
        put_char('|');
        put_char('\n');
    }
}

static void touch_fs_file(const char* name) {
    static const uint8_t empty_data[1] = {0};

    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (!pfs_write_file(name, empty_data, 0)) {
        print("touch: failed\n");
        return;
    }

    print("Created ");
    print(name);
    put_char('\n');
}

static void write_fs_file(const char* name, const char* text) {
    uint32_t len = 0;

    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (!text || !text[0]) {
        print("write: text is required\n");
        return;
    }

    while (text[len]) {
        len++;
        if (len > 480) {
            print("write: max 480 bytes\n");
            return;
        }
    }

    if (!pfs_write_file(name, (const uint8_t*)text, len)) {
        print("write: failed\n");
        return;
    }

    print("Saved ");
    print_int((int)len);
    print(" bytes to ");
    print(name);
    put_char('\n');
}

static void rm_fs_file(const char* name) {
    if (!pfs_is_ready()) {
        print("PFS not ready (disk missing?)\n");
        return;
    }

    if (!pfs_delete_file(name)) {
        print("rm: file not found\n");
        return;
    }

    print("Deleted ");
    print(name);
    put_char('\n');
}

static void execute_command(const char* cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    if (str_equals(cmd, "help")) {
        int lines = (int)(sizeof(g_shell_help_lines) / sizeof(g_shell_help_lines[0]));
        for (int i = 0; i < lines; i++) {
            print(g_shell_help_lines[i]);
            put_char('\n');
        }
        return;
    }

    if (str_equals(cmd, "desktop")) {
        enter_desktop_mode();
        return;
    }

    if (str_equals(cmd, "clear")) {
        clear_screen();
        return;
    }

    if (str_equals(cmd, "about")) {
        print("MyOS kernel shell\n");
        print("Intermediate kernel stage: memory + shell utilities\n");
        print("Tired? Do it anyway.\nUnmotivated? Do it anyway.\nDoubting yourself? Do it anyway.\n");
        return;
    }

    if (str_equals(cmd, "time")) {
        print_time_now();
        return;
    }

    if (str_starts_with(cmd, "set time ")) {
        if (parse_set_time(cmd + 9)) {
            print("Custom time set.\n");
        } else {
            print("Usage: set time HH:MM:SS\n");
        }
        return;
    }

    if (str_equals(cmd, "calendar")) {
        print_calendar();
        return;
    }

    if (str_starts_with(cmd, "echo ")) {
        print(cmd + 5);
        put_char('\n');
        return;
    }

    if (str_equals(cmd, "heap")) {
        print_heap_stats();
        return;
    }

    if (str_starts_with(cmd, "kmalloc ")) {
        int bytes = 0;
        if (!parse_uint(cmd + 8, &bytes) || bytes <= 0) {
            print("Usage: kmalloc N\n");
            return;
        }

        void* ptr = kmalloc((uint32_t)bytes);
        if (!ptr) {
            print("kmalloc failed: out of heap memory\n");
            return;
        }

        print("Allocated ");
        print_int(bytes);
        print(" bytes at ");
        print_hex((uint32_t)ptr);
        put_char('\n');
        return;
    }

    if (str_equals(cmd, "memmap")) {
        print_memory_map();
        return;
    }

    if (str_equals(cmd, "paging")) {
        print("Paging: ");
        if (paging_is_enabled()) {
            print("enabled\n");
        } else {
            print("disabled\n");
        }
        return;
    }

    if (str_equals(cmd, "ticks")) {
        print("Ticks: ");
        print_int((int)timer_get_ticks());
        print("  Hz: ");
        print_int((int)timer_get_hz());
        put_char('\n');
        return;
    }

    if (str_equals(cmd, "uptime")) {
        print("Uptime: ");
        print_int((int)timer_get_uptime_seconds());
        print(" seconds\n");
        return;
    }

    if (str_equals(cmd, "ls")) {
        print_fs_list();
        return;
    }

    if (str_starts_with(cmd, "cat ")) {
        char name[PFS_NAME_MAX];
        if (!parse_token(cmd + 4, name, PFS_NAME_MAX)) {
            print("Usage: cat FILE\n");
            return;
        }
        cat_fs_file(name);
        return;
    }

    if (str_starts_with(cmd, "hexdump ")) {
        char name[PFS_NAME_MAX];
        if (!parse_token(cmd + 8, name, PFS_NAME_MAX)) {
            print("Usage: hexdump FILE\n");
            return;
        }
        hexdump_fs_file(name);
        return;
    }

    if (str_starts_with(cmd, "touch ")) {
        char name[PFS_NAME_MAX];
        if (!parse_token(cmd + 6, name, PFS_NAME_MAX)) {
            print("Usage: touch FILE\n");
            return;
        }
        touch_fs_file(name);
        return;
    }

    if (str_starts_with(cmd, "write ")) {
        char name[PFS_NAME_MAX];
        const char* text = 0;
        if (!parse_two_tokens_and_rest(cmd + 6, name, PFS_NAME_MAX, &text)) {
            print("Usage: write FILE TEXT\n");
            return;
        }
        if (!text || !text[0]) {
            print("Usage: write FILE TEXT\n");
            return;
        }
        write_fs_file(name, text);
        return;
    }

    if (str_starts_with(cmd, "rm ")) {
        char name[PFS_NAME_MAX];
        if (!parse_token(cmd + 3, name, PFS_NAME_MAX)) {
            print("Usage: rm FILE\n");
            return;
        }
        rm_fs_file(name);
        return;
    }

    if (str_equals(cmd, "rls")) {
        print("RAMFS files:\n");
        int count = ramfs_count();
        for (int i = 0; i < count; i++) {
            ramfs_entry_t entry;
            if (!ramfs_get_entry(i, &entry)) {
                continue;
            }
            print("  ");
            print(entry.name);
            print("  ");
            print_int((int)entry.size);
            print(" bytes\n");
        }
        return;
    }

    if (str_starts_with(cmd, "bg run ")) {
        char name[SCHED_TASK_NAME_MAX];
        if (!parse_bg_name(cmd + 7, name)) {
            print("Usage: bg run TASK_NAME|all\n");
            return;
        }

        if (str_equals(name, "all")) {
            int started = 0;
            if (scheduler_start_named_task("task1") > 0) started++;
            if (scheduler_start_named_task("task2") > 0) started++;
            if (scheduler_start_named_task("task3") > 0) started++;
            print("Started ");
            print_int(started);
            print(" tasks\n");
            return;
        }

        int id = scheduler_start_named_task(name);
        if (id > 0) {
            print("Started ");
            print(name);
            print(" as #");
            print_int(id);
            put_char('\n');
        } else if (id == -3) {
            print("Task already running\n");
        } else {
            print("Could not start task\n");
        }
        return;
    }

    if (str_equals(cmd, "bg list")) {
        print_scheduler_tasks();
        return;
    }

    if (str_starts_with(cmd, "bg stop ")) {
        char arg[SCHED_TASK_NAME_MAX];
        if (!parse_bg_name(cmd + 8, arg)) {
            print("Usage: bg stop ID|TASK_NAME|all\n");
            return;
        }

        if (str_equals(arg, "all")) {
            scheduler_stop_all();
            print("All background tasks stopped\n");
            return;
        }

        int id = 0;
        if (parse_uint(arg, &id)) {
            if (scheduler_stop_task_by_id(id)) {
                print("Stopped task #");
                print_int(id);
                put_char('\n');
            } else {
                print("Task id not found\n");
            }
            return;
        }

        if (scheduler_stop_task_by_name(arg)) {
            print("Stopped ");
            print(arg);
            put_char('\n');
        } else {
            print("Task not found\n");
        }
        return;
    }

    if (str_equals(cmd, "reboot")) {
        print("Rebooting...\n");
        outb(0x64, 0xFE);
        __asm__ volatile("cli");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    if (str_equals(cmd, "shutdown")) {
        print("Shutting down...\n");
        outw(0x604, 0x2000);
        __asm__ volatile("cli");
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    print("Unknown command: ");
    print(cmd);
    print("\nType 'help' to list commands.\n");
}

int shell_get_help_lines(const char** out_lines, int max_lines) {
    int total = (int)(sizeof(g_shell_help_lines) / sizeof(g_shell_help_lines[0]));
    if (!out_lines || max_lines <= 0) {
        return total;
    }

    int n = total < max_lines ? total : max_lines;
    for (int i = 0; i < n; i++) {
        out_lines[i] = g_shell_help_lines[i];
    }
    return n;
}

void shell_execute_command_direct(const char* cmd) {
    execute_command(cmd);
}

void shell_init() {
    input_len = 0;
    print_prompt();
}

void shell_input_char(char c) {
    if (desktop_mode) {
        if (c == 27) {
            exit_desktop_mode();
        }
        return;
    }

    if (c == '\t') {
        return;
    }

    if (c == '\b') {
        if (input_len > 0) {
            input_len--;
            input_buffer[input_len] = '\0';
            put_char('\b');
        }
        return;
    }

    if (c == '\n') {
        put_char('\n');
        input_buffer[input_len] = '\0';
        execute_command(input_buffer);
        input_len = 0;
        print_prompt();
        return;
    }

    if (c < 32 || c > 126) {
        return;
    }

    if (input_len < SHELL_MAX_INPUT - 1) {
        input_buffer[input_len] = c;
        input_len++;
        put_char(c);
    }
}

void shell_mouse_event(int x, int y,
                       int left_down, int right_down,
                       int left_pressed, int left_released,
                       int right_pressed, int right_released) {
    (void)right_down;
    (void)left_pressed;
    (void)right_pressed;

    if (!desktop_mode) {
        return;
    }

    int old_x = desktop_mouse_x;
    int old_y = desktop_mouse_y;
    desktop_mouse_x = x;
    desktop_mouse_y = y;

    int needs_redraw = (old_x != x) || (old_y != y) || left_released || right_released || fs_drag_active || fs_resize_active;

    if (fs_window_open && left_down &&
        in_rect(x, y, fs_window_x + 1, fs_window_y, fs_window_w - 8, 1) &&
        !fs_drag_active) {
        fs_drag_active = 1;
        fs_drag_off_x = x - fs_window_x;
        fs_drag_off_y = y - fs_window_y;
    }

    if (fs_window_open && left_down &&
        in_rect(x, y, fs_window_x + fs_window_w - 2, fs_window_y + fs_window_h - 1, 2, 1) &&
        !fs_drag_active) {
        fs_resize_active = 1;
    }

    if (fs_drag_active && left_down) {
        fs_window_x = x - fs_drag_off_x;
        fs_window_y = y - fs_drag_off_y;
        needs_redraw = 1;
    }

    if (fs_resize_active && left_down) {
        int new_w = (x - fs_window_x) + 2;
        int new_h = (y - fs_window_y) + 1;

        if (new_w < 20) new_w = 20;
        if (new_h < 8) new_h = 8;
        if (new_w > 60) new_w = 60;
        if (new_h > 18) new_h = 18;

        fs_window_w = new_w;
        fs_window_h = new_h;
        needs_redraw = 1;
    }

    if (left_released) {
        if (fs_drag_active) {
            fs_drag_active = 0;
        }
        if (fs_resize_active) {
            fs_resize_active = 0;
        }

        if (fs_window_open && in_rect(x, y, fs_window_x + fs_window_w - 6, fs_window_y, 5, 1)) {
            fs_window_open = 0;
        }

        uint32_t now = timer_get_ticks();
        int is_double = (now - last_left_click_tick) <= 30;
        last_left_click_tick = now;

        if (is_double && in_rect(x, y, 22, 3, 14, 6)) {
            fs_window_open = 1;
            context_visible = 0;
            needs_redraw = 1;
        }

        if (is_double && in_rect(x, y, 4, 3, 14, 6)) {
            exit_desktop_mode();
            return;
        }
    }

    if (right_released) {
        context_x = x;
        context_y = y;
        context_visible = 1;
        if (in_rect(x, y, 22, 3, 14, 6)) {
            context_target = "Files icon options";
        } else if (in_rect(x, y, 4, 3, 14, 6)) {
            context_target = "Shell icon options";
        } else {
            context_target = "Desktop options";
        }
        needs_redraw = 1;
    }

    uint32_t now_tick = timer_get_ticks();
    if (needs_redraw && (now_tick - last_desktop_redraw_tick >= 2 || left_released || right_released)) {
        desktop_redraw();
    }
}