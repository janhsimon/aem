#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct Camera;
struct ToolState;
struct Viewport;

enum ViewportType
{
  ViewportType_Perspective,
  ViewportType_Top,
  ViewportType_Front,
  ViewportType_Left
};

bool make_viewport(struct Viewport** viewport,
                   enum ViewportType type,
                   uint32_t x,
                   uint32_t y,
                   uint32_t width,
                   uint32_t height,
                   struct ToolState* tool_state);
void destroy_viewport(struct Viewport* viewport);

void viewport_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void viewport_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void viewport_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position);
void viewport_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void viewport_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position);
void viewport_on_mouse_scrolled(struct Viewport* viewport, vec2 mouse_position, float delta);

enum ViewportType viewport_get_type(const struct Viewport* viewport);
const char* viewport_get_type_name(const struct Viewport* viewport);

void viewport_get_position(const struct Viewport* viewport, uint32_t* x, uint32_t* y);
void viewport_set_position(struct Viewport* viewport, uint32_t x, uint32_t y);

void viewport_get_resolution(const struct Viewport* viewport, uint32_t* width, uint32_t* height);
void viewport_set_resolution(struct Viewport* viewport, uint32_t width, uint32_t height);

uint32_t viewport_get_framebuffer_texture(const struct Viewport* viewport);

struct Camera* viewport_get_camera(const struct Viewport* viewport);

bool viewport_is_mouse_looking(const struct Viewport* viewport);

void viewport_render(const struct Viewport* viewport);