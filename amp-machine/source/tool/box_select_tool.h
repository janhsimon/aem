#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Viewport;

// Returns true if the function consumed the event
bool box_select_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool box_select_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void box_select_tool_on_mouse_drag_finished();

void box_select_tool_draw_gizmo(const struct Viewport* viewport);