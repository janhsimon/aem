#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Camera;
struct Viewport;

// Returns true if the function consumed the event
bool rotate_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool rotate_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool rotate_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
bool rotate_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);

void rotate_tool_draw_gizmo(const struct Viewport* viewport);
