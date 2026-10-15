#include "add_cube_tool.h"

#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "tool_state.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <assert.h>
#include <stdint.h>

static bool is_adding = false;

static vec3 bbox_min, bbox_max; // Bounding box of new cube in world space

void add_cube_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
{
  vec2 bbox_min_clip, bbox_max_clip;
  camera_world_to_clip(camera, bbox_min, bbox_min_clip);
  camera_world_to_clip(camera, bbox_max, bbox_max_clip);

  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_min_clip[1] }, corners[0]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_min_clip[1] }, corners[1]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_max_clip[1] }, corners[2]);
  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_max_clip[1] }, corners[3]);
}

void add_cube_tool_on_tool_activate()
{
  scene_deselect_all_blocks();
}

bool add_cube_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  input_use_draw_cursor();

  // Consume the event so we don't fallback to selection functionality
  return true;
}

bool add_cube_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  return false;
}

bool add_cube_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  return false;
}

bool add_cube_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  if (!is_adding)
  {
    is_adding = true;

    // Make initial new cube bbox
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

    glm_vec3_copy(mouse_pos_world, bbox_min);
    glm_vec3_copy(mouse_pos_world, bbox_max);

    // Snap to grid
    bbox_min[0] = roundf(bbox_min[0] /*- 0.5f*/);
    bbox_min[1] = roundf(bbox_min[1] /*- 0.5f*/);
    bbox_min[2] = roundf(bbox_min[2] /*- 0.5f*/);

    bbox_max[0] = roundf(bbox_max[0]) /*+ 0.5f*/;
    bbox_max[1] = roundf(bbox_max[1]) /*+ 0.5f*/;
    bbox_max[2] = roundf(bbox_max[2]) /*+ 0.5f*/;

    // Extend depth-component of volume
    {
      vec3 camera_forward;
      camera_get_forward(camera, camera_forward);
      for (int i = 0; i < 3; ++i)
      {
        if (camera_forward[i] != 0.0f)
        {
          bbox_min[i] -= 1.0f;
        }
      }
    }
  }
  else
  {
    // Extend new cube bbox
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);
    glm_vec3_copy(mouse_pos_world, bbox_max);

    for (int i = 0; i < 3; ++i)
    {
      // Snap to grid
      bbox_min[i] = roundf(bbox_min[i]);
      bbox_max[i] = roundf(bbox_max[i]);

      // Ensure non-zero volume
      /*const float delta = bbox_max[i] - bbox_min[i];
      if (delta == 0.0f)
      {
        bbox_max[i] += 1.0f;
      }*/
    }
  }

  return true;
}

bool add_cube_tool_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  is_adding = false;

  // Check if the cube is valid (ie. has a non-zero volume)
  for (int i = 0; i < 3; ++i)
  {
    if (fabsf(bbox_max[i] - bbox_min[i]) < 0.1f)
    {
      return true;
    }
  }

  // Correct min vs max
  for (int i = 0; i < 3; ++i)
  {
    if (bbox_min[i] > bbox_max[i])
    {
      const float temp = bbox_max[i];
      bbox_max[i] = bbox_min[i];
      bbox_min[i] = temp;
    }
  }

  scene_add_block(bbox_min, bbox_max);
  scene_select_block(scene_get_last_added_block_index());

  activate_tool(Tool_Scale);

  return true;
}

void add_cube_tool_get_bbox(vec3 min, vec3 max)
{
  glm_vec3_copy(bbox_min, min);
  glm_vec3_copy(bbox_max, max);
}

bool add_cube_tool_should_draw_gizmo(bool* is_cube_valid)
{
  *is_cube_valid = true;
  for (int i = 0; i < 3; ++i)
  {
    if (fabsf(bbox_max[i] - bbox_min[i]) < 0.1f)
    {
      *is_cube_valid = false;
      break;
    }
  }

  return get_active_tool() == Tool_AddCube && is_adding;
}