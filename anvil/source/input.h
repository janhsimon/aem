#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

enum MouseButton
{
  MouseButton_None,
  MouseButton_Left,
  MouseButton_Right,
  MouseButton_Middle
};

struct GLFWwindow;
struct ToolState;

bool load_input(struct GLFWwindow* window, struct ToolState* tool_state);
void destroy_input();

void input_mouse_button_callback(struct GLFWwindow* window, int button, int action, int mods);
void input_mouse_position_callback(struct GLFWwindow* window, double x, double y);
void input_mouse_scroll_callback(struct GLFWwindow* window, double x, double y);
void input_key_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods);

void input_get_camera_movement(vec3 move);
bool input_is_duplicate_key_down();

void input_use_default_cursor();
void input_use_mouse_look_cursor();
void input_use_pan_cursor();
void input_use_move_cursor();
void input_use_scale_cursor(uint32_t direction);
void input_use_draw_cursor();