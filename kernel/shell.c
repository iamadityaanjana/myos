#include "shell.h"

#include "screen.h"
#include "io.h"
#include "printk.h"
#include "memory.h"
#include "paging.h"
#include "rtc.h"

#define SHELL_MAX_INPUT 64

static char input_buffer[SHELL_MAX_INPUT];
static int input_len = 0;

static int custom_time_enabled = 0;
static uint8_t custom_hour = 0;
static uint8_t custom_minute = 0;
static uint8_t custom_second = 0;

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
    rtc_datetime_t now;
    rtc_read_datetime(&now);

    int hour = now.hour;
    int minute = now.minute;
    int second = now.second;

    if (custom_time_enabled) {
        hour = custom_hour;
        minute = custom_minute;
        second = custom_second;
    }

    print("Time: ");
    print_two_digits(hour);
    put_char(':');
    print_two_digits(minute);
    put_char(':');
    print_two_digits(second);
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

    custom_hour = (uint8_t)h;
    custom_minute = (uint8_t)m;
    custom_second = (uint8_t)s;
    custom_time_enabled = 1;
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

static void execute_command(const char* cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    if (str_equals(cmd, "help")) {
        print("Commands:\n");
        print("  help      - list commands\n");
        print("  clear     - clear screen\n");
        print("  about     - about MyOS\n");
        print("  shutdown  - power off emulator\n");
        print("  reboot    - reboot CPU\n");
        print("  echo TXT  - print text\n");
        print("  time      - show current time/date\n");
        print("  set time HH:MM:SS - override displayed time\n");
        print("  calendar  - show current month calendar\n");
        print("  heap      - show heap usage\n");
        print("  kmalloc N - allocate N bytes\n");
        print("  memmap    - show kernel memory map\n");
        print("  paging    - show paging status\n");
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

void shell_init() {
    input_len = 0;
    print_prompt();
}

void shell_input_char(char c) {
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