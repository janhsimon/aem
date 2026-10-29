#include "add_cube_tool.h"

#include "grid.h"
#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "tool_state.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/vec2.h>
#include <cglm/vec3.h>

static bool is_adding = false;

static vec3 bbox_from, bbox_to; // Bounding box of new cube in world space

void add_cube_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
{
  vec2 bbox_from_clip, bbox_to_clip;
  camera_world_to_clip(camera, bbox_from, bbox_from_clip);
  camera_world_to_clip(camera, bbox_to, bbox_to_clip);

  glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_from_clip[1] }, corners[0]);
  glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_from_clip[1] }, corners[1]);
  glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_to_clip[1] }, corners[2]);
  glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_to_clip[1] }, corners[3]);
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

    camera_screen_to_world_2d(camera, mouse_position, bbox_from);
    glm_vec3_copy(bbox_from, bbox_to);

    // Extend depth-component of volume
    {
      vec3 last_selection_bbox_min, last_selection_bbox_max;
      get_last_selection_bbox(last_selection_bbox_min, last_selection_bbox_max);
      vec3 camera_forward;
      camera_get_forward(camera, camera_forward);
      for (int i = 0; i < 3; ++i)
      {
        if (camera_forward[i] != 0.0f)
        {
          bbox_from[i] = last_selection_bbox_min[i];
          bbox_to[i] = last_selection_bbox_max[i];
        }
      }
    }

    // Snap to grid
    grid_snap(bbox_from, bbox_from);
    grid_snap(bbox_to, bbox_to);
  }
  else
  {
    // Extend new cube bbox
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 camera_forward;
    camera_get_forward(camera, camera_forward);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);
    for (int i = 0; i < 3; ++i)
    {
      if (camera_forward[i] == 0.0f)
      {
        bbox_to[i] = mouse_pos_world[i];
      }
    }

    vec3 unsnapped_to;
    glm_vec3_copy(bbox_to, unsnapped_to);

    grid_snap(bbox_to, bbox_to);

    const float min_grid_size = get_min_grid_size();
    for (int i = 0; i < 3; ++i)
    {
      if (camera_forward[i] == 0.0f)
      {
        if (fabsf(bbox_to[i] - bbox_from[i]) < min_grid_size)
        {
          if (unsnapped_to[i] < bbox_from[i])
          {
            bbox_to[i] = bbox_from[i] - min_grid_size;
          }
          else
          {
            bbox_to[i] = bbox_from[i] + min_grid_size;
          }
        }
      }
    }
  }

  return true;
}

static void add_cube_tool_get_bbox(vec3 min, vec3 max)
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

bool add_cube_tool_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  is_adding = false;

  vec3 bbox_min, bbox_max;
  add_cube_tool_get_bbox(bbox_min, bbox_max);

  scene_add_block(bbox_min, bbox_max);
  scene_select_block(scene_get_last_added_block_index());

  activate_tool(Tool_Scale);

  return true;
}

bool add_cube_tool_should_draw_gizmo()
{
  return get_active_tool() == Tool_AddCube && is_adding;
}