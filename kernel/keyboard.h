#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init();
void keyboard_irq_handler();
void keyboard_process_pending();

#endif
