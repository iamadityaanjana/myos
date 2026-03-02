/* kernel/kernel.c - Minimal OS Kernel */

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  ((unsigned short *)0xB8000)

/* Color attributes (background << 4 | foreground) */
#define COLOR_WHITE_ON_BLACK  0x0F
#define COLOR_GREEN_ON_BLACK  0x0A
#define COLOR_CYAN_ON_BLACK   0x0B

static int cursor_col = 0;
static int cursor_row = 0;

/* Clear the entire screen */
void clear_screen(void) {
    unsigned short blank = (COLOR_WHITE_ON_BLACK << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        VGA_MEMORY[i] = blank;
    cursor_col = 0;
    cursor_row = 0;
}

/* Print a single character with color */
void putchar_color(char c, unsigned char color) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        return;
    }
    if (cursor_row >= VGA_HEIGHT) {
        /* Scroll: move lines up */
        for (int row = 1; row < VGA_HEIGHT; row++)
            for (int col = 0; col < VGA_WIDTH; col++)
                VGA_MEMORY[(row - 1) * VGA_WIDTH + col] =
                    VGA_MEMORY[row * VGA_WIDTH + col];
        /* Clear last line */
        unsigned short blank = (color << 8) | ' ';
        for (int col = 0; col < VGA_WIDTH; col++)
            VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;
        cursor_row = VGA_HEIGHT - 1;
    }
    int index = cursor_row * VGA_WIDTH + cursor_col;
    VGA_MEMORY[index] = ((unsigned short)color << 8) | (unsigned char)c;
    cursor_col++;
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
}

/* Print a string with color */
void print_color(const char *str, unsigned char color) {
    for (int i = 0; str[i] != '\0'; i++)
        putchar_color(str[i], color);
}

/* Print a string (white on black) */
void print(const char *str) {
    print_color(str, COLOR_WHITE_ON_BLACK);
}

/* Print a string and newline */
void println(const char *str) {
    print(str);
    putchar_color('\n', COLOR_WHITE_ON_BLACK);
}

/* -------------------------------------------------------
   Kernel entry point
   ------------------------------------------------------- */
void kernel_main(void) {
    clear_screen();

    print_color("================================\n", COLOR_CYAN_ON_BLACK);
    print_color("         MyOS v0.1              \n", COLOR_GREEN_ON_BLACK);
    print_color("================================\n", COLOR_CYAN_ON_BLACK);
    println("");
    println("Kernel loaded successfully.");
    println("");
    print_color("Ready.\n", COLOR_GREEN_ON_BLACK);

    /* Hang: disable interrupts then halt — no IDT set up, any interrupt
       would triple-fault and reset the machine. */
    __asm__ volatile(
        "cli          \n"
        ".Lhalt:       \n"
        "hlt           \n"
        "jmp .Lhalt    \n"
    );
}
