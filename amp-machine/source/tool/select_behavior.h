#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct Viewport;

// Returns true if the function consumed the event
bool select_behavior_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
bool select_behavior_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
bool select_behavior_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void select_behavior_on_mouse_drag_finished();

// Draws the wireframe overlay and the selection box gizmo
void select_behavior_draw_gizmo(const struct Viewport* viewport);