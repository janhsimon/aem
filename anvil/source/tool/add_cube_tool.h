#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Viewport;

void add_cube_tool_on_tool_activate();

// Returns true if the function consumed the event
bool add_cube_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
bool add_cube_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);

void add_cube_tool_draw_gizmo(const struct Viewport* viewport);