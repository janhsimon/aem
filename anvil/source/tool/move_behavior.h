#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct Viewport;

// Returns true if the function consumed the event
bool move_behavior_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void move_behavior_on_mouse_button_clicked();
bool move_behavior_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void move_behavior_on_mouse_drag_finished();

void move_behavior_reanchor(struct Viewport* viewport, vec2 mouse_position);