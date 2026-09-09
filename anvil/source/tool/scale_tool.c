#include "scale_tool.h"

#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <assert.h>
#include <stdint.h>

#define SELECTION_RADIUS 6.0f
#define CONTROL_POINT_DISTANCE 15.0f

#define CONTROL_POINT_BOTTOM_RIGHT 0
#define CONTROL_POINT_BOTTOM_LEFT 1
#define CONTROL_POINT_TOP_LEFT 2
#define CONTROL_POINT_TOP_RIGHT 3
#define CONTROL_POINT_BOTTOM 4
#define CONTROL_POINT_LEFT 5
#define CONTROL_POINT_TOP 6
#define CONTROL_POINT_RIGHT 7

static bool is_moving, is_scaling;
static uint32_t scale_control_point_index;
static vec3 move_delta;

void scale_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
{
  vec3 bbox_min_world, bbox_max_world;
  scene_get_selection_bbox_world(bbox_min_world, bbox_max_world);

  vec2 bbox_min_clip, bbox_max_clip;
  camera_world_to_clip(camera, bbox_min_world, bbox_min_clip);
  camera_world_to_clip(camera, bbox_max_world, bbox_max_clip);

  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_min_clip[1] }, corners[0]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_min_clip[1] }, corners[1]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_max_clip[1] }, corners[2]);
  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_max_clip[1] }, corners[3]);
}

void scale_tool_corners_to_control_points_clip(vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[8])
{
  // Determine center between corners
  vec2 center = GLM_VEC2_ZERO_INIT;
  {
    for (uint32_t corner_index = 0; corner_index < 4; ++corner_index)
    {
      glm_vec2_add(center, in_corners[corner_index], center);
    }

    glm_vec2_scale(center, 0.25f, center);
  }

  // Invert viewport size
  vec2 inv_viewport_size;
  inv_viewport_size[0] = 1.0f / viewport_size[0];
  inv_viewport_size[1] = 1.0f / viewport_size[1];

  // Set corner control points, nudged away from center
  for (uint32_t corner_index = 0; corner_index < 4; ++corner_index)
  {
    glm_vec2_copy(in_corners[corner_index], out_control_points[corner_index]);

    vec2 center_to_corner;
    glm_vec2_sub(out_control_points[corner_index], center, center_to_corner);

    for (uint32_t axis_index = 0; axis_index < 2; ++axis_index)
    {
      if (center_to_corner[axis_index] < 0.0f)
      {
        out_control_points[corner_index][axis_index] -= inv_viewport_size[axis_index] * CONTROL_POINT_DISTANCE;
      }
      else if (center_to_corner[axis_index] > 0.0f)
      {
        out_control_points[corner_index][axis_index] += inv_viewport_size[axis_index] * CONTROL_POINT_DISTANCE;
      }
    }
  }

  // Add in mid control points between corner control points
  {
    glm_vec2_add(out_control_points[0], out_control_points[1], out_control_points[4]);
    glm_vec2_add(out_control_points[1], out_control_points[2], out_control_points[5]);
    glm_vec2_add(out_control_points[2], out_control_points[3], out_control_points[6]);
    glm_vec2_add(out_control_points[3], out_control_points[0], out_control_points[7]);

    for (uint32_t vertex_index = 4; vertex_index < 8; ++vertex_index)
    {
      glm_vec2_scale(out_control_points[vertex_index], 0.5f, out_control_points[vertex_index]);
    }
  }
}

static void
corners_to_control_points_screen(vec2 viewport_pos, vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[8])
{
  scale_tool_corners_to_control_points_clip(viewport_size, in_corners, out_control_points);

  // Transform from clip to screen space
  for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
  {
    glm_vec2_adds(out_control_points[control_point_index], 1.0f, out_control_points[control_point_index]);
    glm_vec2_scale(out_control_points[control_point_index], 0.5f, out_control_points[control_point_index]);
    glm_vec2_mul(out_control_points[control_point_index], viewport_size, out_control_points[control_point_index]);
    glm_vec2_add(out_control_points[control_point_index], viewport_pos, out_control_points[control_point_index]);
  }
}

bool scale_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  is_moving = is_scaling = false;

  if (button != MouseButton_Left)
  {
    return false;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  // Check for control points first
  if (scene_has_selection())
  {
    vec2 corners[4];
    scale_tool_calc_selection_corners(camera, corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[8];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < 5.0f)
      {
        is_scaling = true;
        scale_control_point_index = control_point_index;
        return true;
      }
    }
  }

  // Check if an unselected block is close and, if so, select it first
  // This is to enable the user to select and drag a block with one motion
  // Instead of having to click first, release the mouse button and then drag
  bool ready_for_move = false;
  {
    float distance;
    uint32_t closest_block_index;

    // Prioritize finding any selected block in range
    {
      const bool selected_block_found =
        scene_get_closest_block_to(viewport, mouse_position, SceneGetClosestBlockFilter_OnlySelected,
                                   &closest_block_index, &distance);
      if (!selected_block_found || distance > SELECTION_RADIUS)
      {
        // If there was no selected block close by, fallback to the closest unselected block
        if (!scene_get_closest_block_to(viewport, mouse_position, SceneGetClosestBlockFilter_OnlyUnselected,
                                        &closest_block_index, &distance))
        {
          return false;
        }
      }
    }

    if (distance <= SELECTION_RADIUS)
    {
      if (!scene_is_block_selected(closest_block_index))
      {
        scene_deselect_all_blocks();
        scene_select_block(closest_block_index);
      }

      input_use_move_cursor();
      ready_for_move = true;
    }
  }

  if (!ready_for_move)
  {
    return false;
  }

  // Start a move
  {
    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

    vec3 anchor;
    scene_get_selection_first_vertex(anchor);
    glm_vec3_sub(anchor, mouse_pos_world, move_delta);

    if (input_is_duplicate_key_down())
    {
      scene_duplicate_selection();
    }

    is_moving = true;
  }

  return true;
}

bool scale_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Check for control points
  {
    vec2 corners[4];
    scale_tool_calc_selection_corners(viewport_get_camera(viewport), corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[8];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < 5.0f)
      {
        // Consume the mouse click
        return true;
      }
    }
  }

  return false;
}

bool scale_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  // Check for corner control points
  if (scene_has_selection())
  {
    struct Camera* camera = viewport_get_camera(viewport);

    vec2 corners[4];
    scale_tool_calc_selection_corners(camera, corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[8];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < 5.0f)
      {
        // - shape
        if (control_point_index == CONTROL_POINT_LEFT || control_point_index == CONTROL_POINT_RIGHT)
        {
          input_use_scale_cursor(0);
        }
        // | shape
        else if (control_point_index == CONTROL_POINT_TOP || control_point_index == CONTROL_POINT_BOTTOM)
        {
          input_use_scale_cursor(1);
        }
        // \ shape
        else if (control_point_index == CONTROL_POINT_TOP_LEFT || control_point_index == CONTROL_POINT_BOTTOM_RIGHT)
        {
          input_use_scale_cursor(2);
        }
        // / shape
        else if (control_point_index == CONTROL_POINT_TOP_RIGHT || control_point_index == CONTROL_POINT_BOTTOM_LEFT)
        {
          input_use_scale_cursor(3);
        }

        return true;
      }
    }
  }

  float distance;
  uint32_t closest_block_index;
  if (!scene_get_closest_block_to(viewport, mouse_position, SceneGetClosestBlockFilter_All, &closest_block_index,
                                  &distance))
  {
    return false;
  }

  if (distance > SELECTION_RADIUS)
  {
    return false;
  }

  input_use_move_cursor();
  return true;
}

bool scale_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left || !scene_has_selection)
  {
    return false;
  }

  if (is_moving)
  {
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

    vec3 m;
    glm_vec3_add(mouse_pos_world, move_delta, m);

    // Snap to grid
    m[0] = roundf(m[0]);
    m[1] = roundf(m[1]);
    m[2] = roundf(m[2]);
    scene_move_selection(m);

    return true;
  }

  if (is_scaling)
  {
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

    // Snapping
    mouse_pos_world[0] = roundf(mouse_pos_world[0]);
    mouse_pos_world[1] = roundf(mouse_pos_world[1]);
    mouse_pos_world[2] = roundf(mouse_pos_world[2]);

    vec3 bbox_min_world, bbox_max_world;
    scene_get_selection_bbox_world(bbox_min_world, bbox_max_world);

    uint32_t axis_index_horizontal, axis_index_vertical;
    {
      const enum ViewportType type = viewport_get_type(viewport);
      if (type == ViewportType_Top)
      {
        axis_index_horizontal = 0;
        axis_index_vertical = 2;
      }
      else if (type == ViewportType_Front)
      {
        axis_index_horizontal = 0;
        axis_index_vertical = 1;
      }
      else if (type == ViewportType_Left)
      {
        axis_index_horizontal = 2;
        axis_index_vertical = 1;
      }
      else
      {
        assert(false);
        return false;
      }
    }

    // Left
    if (scale_control_point_index == CONTROL_POINT_LEFT || scale_control_point_index == CONTROL_POINT_BOTTOM_LEFT ||
        scale_control_point_index == CONTROL_POINT_TOP_LEFT)
    {
      bbox_max_world[axis_index_horizontal] = mouse_pos_world[axis_index_horizontal];

      if (bbox_max_world[axis_index_horizontal] < bbox_min_world[axis_index_horizontal] + 0.1f)
      {
        bbox_max_world[axis_index_horizontal] = bbox_min_world[axis_index_horizontal] + 0.1f;
      }
    }

    // Right
    if (scale_control_point_index == CONTROL_POINT_RIGHT || scale_control_point_index == CONTROL_POINT_BOTTOM_RIGHT ||
        scale_control_point_index == CONTROL_POINT_TOP_RIGHT)
    {
      bbox_min_world[axis_index_horizontal] = mouse_pos_world[axis_index_horizontal];

      if (bbox_min_world[axis_index_horizontal] > bbox_max_world[axis_index_horizontal] - 0.1f)
      {
        bbox_min_world[axis_index_horizontal] = bbox_max_world[axis_index_horizontal] - 0.1f;
      }
    }

    // Top
    if (scale_control_point_index == CONTROL_POINT_TOP || scale_control_point_index == CONTROL_POINT_TOP_LEFT ||
        scale_control_point_index == CONTROL_POINT_TOP_RIGHT)
    {
      bbox_max_world[axis_index_vertical] = mouse_pos_world[axis_index_vertical];

      if (bbox_max_world[axis_index_vertical] < bbox_min_world[axis_index_vertical] + 0.1f)
      {
        bbox_max_world[axis_index_vertical] = bbox_min_world[axis_index_vertical] + 0.1f;
      }
    }

    // Bottom
    if (scale_control_point_index == CONTROL_POINT_BOTTOM || scale_control_point_index == CONTROL_POINT_BOTTOM_LEFT ||
        scale_control_point_index == CONTROL_POINT_BOTTOM_RIGHT)
    {
      bbox_min_world[axis_index_vertical] = mouse_pos_world[axis_index_vertical];

      if (bbox_min_world[axis_index_vertical] > bbox_max_world[axis_index_vertical] - 0.1f)
      {
        bbox_min_world[axis_index_vertical] = bbox_max_world[axis_index_vertical] - 0.1f;
      }
    }

    // Fix up the bounding box
    for (uint32_t axis_index = 0; axis_index < 3; ++axis_index)
    {
      if (bbox_min_world[axis_index] > bbox_max_world[axis_index])
      {
        const float swap = bbox_min_world[axis_index];
        bbox_min_world[axis_index] = bbox_max_world[axis_index];
        bbox_max_world[axis_index] = swap;
      }
    }

    scene_scale_selection(bbox_min_world, bbox_max_world);
    return true;
  }

  return false;
}

void scale_tool_reanchor(struct Viewport* viewport, vec2 mouse_position)
{
  if (!is_moving)
  {
    return;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  vec3 anchor;
  scene_get_selection_first_vertex(anchor);
  glm_vec3_sub(anchor, mouse_pos_world, move_delta);
}