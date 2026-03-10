#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "types.h"

#define FB_WIDTH 320
#define FB_HEIGHT 200

void framebuffer_init();
void framebuffer_clear(uint8_t color_index);
void framebuffer_put_pixel(int x, int y, uint8_t color_index);
void framebuffer_fill_rect(int x, int y, int w, int h, uint8_t color_index);
void framebuffer_present();

#endif
