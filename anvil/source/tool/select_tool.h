#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Viewport;

void select_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4]); // In clip space

// Returns true if the function consumed the event
bool select_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool select_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void select_tool_on_mouse_drag_finished();

struct Viewport* select_tool_get_active_viewport(); // NULL is not currently selecting