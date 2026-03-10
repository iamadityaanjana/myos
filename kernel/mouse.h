#ifndef MOUSE_H
#define MOUSE_H

void mouse_init();
void mouse_irq_handler();
void mouse_process_pending();
void mouse_get_position(int* x, int* y);
void mouse_get_buttons(int* left_down,
					   int* right_down,
					   int* left_pressed,
					   int* left_released,
					   int* right_pressed,
					   int* right_released);
void mouse_set_bounds(int max_x, int max_y);
void mouse_set_motion_divider(int x_div, int y_div);
void mouse_set_shell_events_enabled(int enabled);

#endif
