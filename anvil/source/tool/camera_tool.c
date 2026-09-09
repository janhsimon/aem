#include "camera_tool.h"

#include "input.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/vec3.h>

static vec3 pan_anchor; // In world space

void camera_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Middle)
  {
    return;
  }

  camera_screen_to_world_2d(viewport_get_camera(viewport), mouse_position, pan_anchor);
  input_use_pan_cursor();
}

void camera_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Middle)
  {
    return;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  vec3 delta;
  glm_vec3_sub(pan_anchor, mouse_pos_world, delta);

  camera_pan_2d(camera, delta);
}

void camera_tool_on_mouse_scrolled(struct Viewport* viewport, vec2 mouse_position, float delta)
{
  struct Camera* camera = viewport_get_camera(viewport);
  camera_zoom_2d(camera, mouse_position, -delta);

  // Re-anchor a potential simultaneous pan operation
  camera_screen_to_world_2d(camera, mouse_position, pan_anchor);
}