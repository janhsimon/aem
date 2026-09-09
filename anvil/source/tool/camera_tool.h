#pragma once

#include <cglm/types.h>

struct Viewport;

void camera_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void camera_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void camera_tool_on_mouse_scrolled(struct Viewport* viewport, vec2 mouse_position, float delta);