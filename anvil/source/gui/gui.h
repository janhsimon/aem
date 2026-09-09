#pragma once

#include <stdbool.h>

struct GLFWwindow;
struct ToolState;
struct Viewport;

void init_gui(struct GLFWwindow* window, struct ToolState* tool_state, void (*file_open_callback)());

// bool is_mouse_consumed();
// bool is_keyboard_consumed();
// bool is_mouse_over_guizmo();

struct Viewport* gui_get_hovered_viewport(); // Returns NULL if none

void update_gui(struct Viewport* viewports[4]);
void render_gui();

void destroy_gui();