#include "printk.h"
#include "screen.h"

void print_int(int n) {
    char buffer[16];
    int i = 0;
    unsigned int value;

    if (n == 0) {
        put_char('0');
        return;
    }

    if (n < 0) {
        put_char('-');
        value = (unsigned int)(-(n + 1)) + 1;
    } else {
        value = (unsigned int)n;
    }

    while (value > 0) {
        buffer[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        put_char(buffer[j]);
    }
}

void print_hex(uint32_t n) {
    const char* hex = "0123456789ABCDEF";
    print("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t digit = (uint8_t)((n >> (i * 4)) & 0xF);
        put_char(hex[digit]);
    }
}
