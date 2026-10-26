#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Camera;
struct Viewport;

void add_cube_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4]); // In clip space

void add_cube_tool_on_tool_activate();

// Returns true if the function consumed the event
bool add_cube_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
bool add_cube_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool add_cube_tool_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);

bool add_cube_tool_should_draw_gizmo();