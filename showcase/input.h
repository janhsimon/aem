#pragma once

#include <cglm/types.h>

#include <stdbool.h>

// Callbacks
void on_mouse_move(double x, double y);
void on_mouse_button_down(int button);
void on_mouse_button_up(int button);
void on_key_down(int key);
void on_key_up(int key);

void get_mouse_delta(double* x, double* y);
bool get_shoot_button_down();
bool get_exit_key_down();
void get_move_vector(vec3 move, bool* moving);
bool get_reload_key_down();
bool get_debug_key_up();

void reset_mouse_move();