#ifndef SHELL_H
#define SHELL_H

void shell_init();
void shell_input_char(char c);
int shell_get_help_lines(const char** out_lines, int max_lines);
void shell_execute_command_direct(const char* cmd);
void shell_mouse_event(int x, int y,
					   int left_down, int right_down,
					   int left_pressed, int left_released,
					   int right_pressed, int right_released);

#endif