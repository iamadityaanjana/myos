#include "framebuffer.h"

#include "io.h"

static uint8_t* const fb_front = (uint8_t*)0xA0000;
static uint8_t fb_back[FB_WIDTH * FB_HEIGHT];

static void set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

void framebuffer_init() {
    // VGA DAC uses 6-bit channels (0..63).
    set_palette_entry(0, 0, 0, 0);
    set_palette_entry(1, 0, 0, 42);
    set_palette_entry(2, 0, 42, 0);
    set_palette_entry(3, 0, 42, 42);
    set_palette_entry(4, 42, 0, 0);
    set_palette_entry(5, 42, 0, 42);
    set_palette_entry(6, 42, 21, 0);
    set_palette_entry(7, 42, 42, 42);
    set_palette_entry(8, 21, 21, 21);
    set_palette_entry(9, 21, 21, 63);
    set_palette_entry(10, 21, 63, 21);
    set_palette_entry(11, 21, 63, 63);
    set_palette_entry(12, 63, 21, 21);
    set_palette_entry(13, 63, 21, 63);
    set_palette_entry(14, 63, 63, 21);
    set_palette_entry(15, 63, 63, 63);

    framebuffer_clear(0);
    framebuffer_present();
}

void framebuffer_clear(uint8_t color_index) {
    for (int i = 0; i < (FB_WIDTH * FB_HEIGHT); i++) {
        fb_back[i] = color_index;
    }
}

void framebuffer_put_pixel(int x, int y, uint8_t color_index) {
    if (x < 0 || y < 0 || x >= FB_WIDTH || y >= FB_HEIGHT) {
        return;
    }

    fb_back[y * FB_WIDTH + x] = color_index;
}

void framebuffer_fill_rect(int x, int y, int w, int h, uint8_t color_index) {
    if (w <= 0 || h <= 0) {
        return;
    }

    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;

    if (x1 > FB_WIDTH) x1 = FB_WIDTH;
    if (y1 > FB_HEIGHT) y1 = FB_HEIGHT;

    for (int yy = y0; yy < y1; yy++) {
        int row = yy * FB_WIDTH;
        for (int xx = x0; xx < x1; xx++) {
            fb_back[row + xx] = color_index;
        }
    }
}

void framebuffer_present() {
    for (int i = 0; i < (FB_WIDTH * FB_HEIGHT); i++) {
        fb_front[i] = fb_back[i];
    }
}
