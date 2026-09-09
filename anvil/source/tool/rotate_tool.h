#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Camera;
struct Viewport;

void rotate_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4]);
void rotate_tool_corners_to_control_points_clip(vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[4]);

// Returns true if the function consumed the event
bool rotate_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool rotate_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool rotate_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
bool rotate_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
