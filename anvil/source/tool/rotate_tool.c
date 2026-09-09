#include "rotate_tool.h"

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

static bool is_moving, is_rotating;
static uint32_t rotate_control_point_index;
static vec3 move_delta;

void rotate_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
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

void rotate_tool_corners_to_control_points_clip(vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[4])
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
}

static void
corners_to_control_points_screen(vec2 viewport_pos, vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[8])
{
  rotate_tool_corners_to_control_points_clip(viewport_size, in_corners, out_control_points);

  // Transform from clip to screen space
  for (uint32_t control_point_index = 0; control_point_index < 4; ++control_point_index)
  {
    glm_vec2_adds(out_control_points[control_point_index], 1.0f, out_control_points[control_point_index]);
    glm_vec2_scale(out_control_points[control_point_index], 0.5f, out_control_points[control_point_index]);
    glm_vec2_mul(out_control_points[control_point_index], viewport_size, out_control_points[control_point_index]);
    glm_vec2_add(out_control_points[control_point_index], viewport_pos, out_control_points[control_point_index]);
  }
}

bool rotate_tool_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  is_moving = is_rotating = false;

  if (button != MouseButton_Left)
  {
    return false;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  // Check for control points first
  if (scene_has_selection())
  {
    vec2 corners[4];
    rotate_tool_calc_selection_corners(camera, corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[4];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 4; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < 5.0f)
      {
        is_rotating = true;
        rotate_control_point_index = control_point_index;
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

bool rotate_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Check for control points
  {
    vec2 corners[4];
    rotate_tool_calc_selection_corners(viewport_get_camera(viewport), corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[4];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 4; ++control_point_index)
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

bool rotate_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  // Check for corner control points
  if (scene_has_selection())
  {
    struct Camera* camera = viewport_get_camera(viewport);

    vec2 corners[4];
    rotate_tool_calc_selection_corners(camera, corners);

    vec2 viewport_pos, viewport_size;
    {
      uint32_t x, y, w, h;
      viewport_get_position(viewport, &x, &y);
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    vec2 control_points[4];
    corners_to_control_points_screen(viewport_pos, viewport_size, corners, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 4; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < 5.0f)
      {
        // / shape
        if (control_point_index == CONTROL_POINT_TOP_LEFT || control_point_index == CONTROL_POINT_BOTTOM_RIGHT)
        {
          input_use_scale_cursor(3);
        }
        // \ shape
        else if (control_point_index == CONTROL_POINT_TOP_RIGHT || control_point_index == CONTROL_POINT_BOTTOM_LEFT)
        {
          input_use_scale_cursor(2);
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

bool rotate_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
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

  if (is_rotating)
  {
    vec3 bbox_min_world, bbox_max_world;
    scene_get_selection_bbox_world(bbox_min_world, bbox_max_world);

    scene_rotate_selection(1.0f);

    return true;
  }

  return false;
}