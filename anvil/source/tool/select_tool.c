#include "select_tool.h"

#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/mat4.h>
#include <cglm/vec2.h>

#include <stdint.h>

#define SELECTION_RADIUS 6.0f

static struct Viewport* selection_viewport = NULL;
static vec2 selection_rect_min, selection_rect_max; // In clip space

bool select_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  scene_deselect_all_blocks();

  vec3 temp;
  camera_screen_to_world_2d(viewport_get_camera(viewport), mouse_position, temp);

  vec2 mouse_position_clip;
  camera_world_to_clip(viewport_get_camera(viewport), temp, mouse_position_clip);

  glm_vec2_copy(mouse_position_clip, selection_rect_min);
  glm_vec2_copy(mouse_position_clip, selection_rect_max);

  return true;
}

bool select_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  selection_viewport = viewport;

  vec3 temp;
  camera_screen_to_world_2d(viewport_get_camera(viewport), mouse_position, temp);

  vec2 mouse_position_clip;
  camera_world_to_clip(viewport_get_camera(viewport), temp, mouse_position_clip);

  glm_vec2_copy(mouse_position_clip, selection_rect_max);

  input_use_draw_cursor();
  return true;
}

void select_tool_on_mouse_drag_finished()
{
  if (!selection_viewport)
  {
    return;
  }

  scene_deselect_all_blocks();

  mat4 viewproj;
  {
    mat4 view, proj;
    camera_calc_view_matrix(viewport_get_camera(selection_viewport), view);
    camera_calc_proj_matrix(viewport_get_camera(selection_viewport), proj);

    glm_mat4_mul(proj, view, viewproj);
    glm_mat4_inv(viewproj, viewproj);
  }

  vec3 rect_min_world, rect_max_world;
  glm_mat4_mulv3(viewproj, selection_rect_min, 1.0f, rect_min_world);
  glm_mat4_mulv3(viewproj, selection_rect_max, 1.0f, rect_max_world);

  // Fix the world-space bounding box
  for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
  {
    if (rect_min_world[axis_index] > rect_max_world[axis_index])
    {
      const float swap = rect_min_world[axis_index];
      rect_min_world[axis_index] = rect_max_world[axis_index];
      rect_max_world[axis_index] = swap;
    }
  }

  if (viewport_get_type(selection_viewport) == ViewportType_Top)
  {
    rect_min_world[1] = -FLT_MAX;
    rect_max_world[1] = FLT_MAX;
  }
  else if (viewport_get_type(selection_viewport) == ViewportType_Front)
  {
    rect_min_world[2] = -FLT_MAX;
    rect_max_world[2] = FLT_MAX;
  }
  else if (viewport_get_type(selection_viewport) == ViewportType_Left)
  {
    rect_min_world[0] = -FLT_MAX;
    rect_max_world[0] = FLT_MAX;
  }

  scene_select_blocks_in_bbox(rect_min_world, rect_max_world);

  selection_viewport = NULL;
}

struct Viewport* select_tool_get_active_viewport()
{
  return selection_viewport;
}

void select_tool_get_rect(vec2 min, vec2 max)
{
  glm_vec2_copy(selection_rect_min, min);
  glm_vec2_copy(selection_rect_max, max);
}