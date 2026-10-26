#include "select_tool.h"

#include "grid.h"
#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/mat4.h>
#include <cglm/vec2.h>

#include <stdint.h>

#define SELECTION_RADIUS 6.0f

static struct Viewport* selection_viewport = NULL; // The viewport that is being selected in
static vec3 bbox_from, bbox_to;                    // Bounding box of selection rectangle in world space

void select_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
{
  vec2 bbox_from_clip, bbox_to_clip;
  camera_world_to_clip(camera, bbox_from, bbox_from_clip);
  camera_world_to_clip(camera, bbox_to, bbox_to_clip);

  glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_from_clip[1] }, corners[0]);
  glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_from_clip[1] }, corners[1]);
  glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_to_clip[1] }, corners[2]);
  glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_to_clip[1] }, corners[3]);
}

bool select_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  if (!input_is_add_to_selection_key_down())
  {
    scene_deselect_all_blocks();
  }

  // Make initial selection bbox
  struct Camera* camera = viewport_get_camera(viewport);

  camera_screen_to_world_2d(camera, mouse_position, bbox_from);
  grid_snap(bbox_from, bbox_from); // Snap to grid
  glm_vec3_copy(bbox_from, bbox_to);

  // Extend depth-component of volume to near-infinity
  {
    vec3 camera_forward;
    camera_get_forward(camera, camera_forward);
    for (int i = 0; i < 3; ++i)
    {
      if (camera_forward[i] != 0.0f)
      {
        bbox_from[i] = -FLT_MAX;
        bbox_to[i] = FLT_MAX;
      }
    }
  }

  selection_viewport = viewport;

  return true;
}

bool select_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Extend selection bbox
  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);
  glm_vec3_copy(mouse_pos_world, bbox_to);

  grid_snap(bbox_to, bbox_to);

  input_use_draw_cursor();
  return true;
}

static void select_tool_get_bbox(vec2 min, vec2 max)
{
  // Fix the bounding box
  for (int i = 0; i < 3; ++i)
  {
    if (bbox_from[i] < bbox_to[i])
    {
      min[i] = bbox_from[i];
      max[i] = bbox_to[i];
    }
    else
    {
      min[i] = bbox_to[i];
      max[i] = bbox_from[i];
    }
  }
}

void select_tool_on_mouse_drag_finished()
{
  if (!selection_viewport)
  {
    return;
  }

  vec3 bbox_min, bbox_max;
  select_tool_get_bbox(bbox_min, bbox_max);
  scene_select_blocks_in_bbox(bbox_min, bbox_max);

  selection_viewport = NULL;
}

struct Viewport* select_tool_get_active_viewport()
{
  return selection_viewport;
}