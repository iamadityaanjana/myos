#include "shell.h"

#include "screen.h"
#include "io.h"

#define SHELL_MAX_INPUT 64

static char input_buffer[SHELL_MAX_INPUT];
static int input_len = 0;

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
        return;
    }

    if (str_equals(cmd, "clear")) {
        clear_screen();
        return;
    }

    if (str_equals(cmd, "about")) {
        print("MyOS kernel shell\n");
        print("Tired? Do it anyway.\nUnmotivated? Do it anyway.\nDoubting yourself? Do it anyway.\n");
        return;
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