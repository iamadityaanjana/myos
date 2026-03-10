#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef void (*keyboard_char_handler_t)(char c);
typedef void (*keyboard_scancode_handler_t)(uint8_t scancode, int extended);

void keyboard_init();
void keyboard_irq_handler();
void keyboard_process_pending();
void keyboard_set_char_handler(keyboard_char_handler_t handler);
void keyboard_set_scancode_handler(keyboard_scancode_handler_t handler);

#endif
