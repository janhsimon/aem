#include "rotate_tool.h"

#include "grid.h"
#include "input.h"
#include "map/map.h"
#include "viewport/camera.h"
#include "viewport/gizmo.h"
#include "viewport/viewport.h"

#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <assert.h>
#include <stdint.h>

#define CONTROL_POINT_SELECTION_RADIUS 5.0f
#define CONTROL_POINT_DISTANCE 15.0f

#define CONTROL_POINT_BOTTOM_RIGHT 0
#define CONTROL_POINT_BOTTOM_LEFT 1
#define CONTROL_POINT_TOP_LEFT 2
#define CONTROL_POINT_TOP_RIGHT 3

static bool is_rotating;
static uint32_t rotate_control_point_index;

static void rotate_tool_calc_selection_corners(struct Camera* camera, vec2 corners[4])
{
  vec3 bbox_min_world, bbox_max_world;
  map_get_selection_bbox_world(bbox_min_world, bbox_max_world);

  vec2 bbox_min_clip, bbox_max_clip;
  camera_world_to_clip(camera, bbox_min_world, bbox_min_clip);
  camera_world_to_clip(camera, bbox_max_world, bbox_max_clip);

  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_min_clip[1] }, corners[0]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_min_clip[1] }, corners[1]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_max_clip[1] }, corners[2]);
  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_max_clip[1] }, corners[3]);
}

static void
rotate_tool_corners_to_control_points_clip(vec2 viewport_size, vec2 in_corners[4], vec2 out_control_points[4])
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
  if (button != MouseButton_Left)
  {
    return false;
  }

  is_rotating = false;

  struct Camera* camera = viewport_get_camera(viewport);

  // Check control points
  if (map_has_selection())
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
      if (dist < CONTROL_POINT_SELECTION_RADIUS)
      {
        is_rotating = true;
        rotate_control_point_index = control_point_index;
        return true;
      }
    }
  }

  return false;
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
      if (dist < CONTROL_POINT_SELECTION_RADIUS)
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
  if (map_has_selection())
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
      if (dist < CONTROL_POINT_DISTANCE)
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

  return false;
}

bool rotate_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left || !is_rotating)
  {
    return false;
  }

  vec3 bbox_min_world, bbox_max_world;
  map_get_selection_bbox_world(bbox_min_world, bbox_max_world);

  map_rotate_selection(1.0f);

  return true;
}

void rotate_tool_draw_gizmo(const struct Viewport* viewport)
{
  if (!map_has_selection())
  {
    return;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  // Get the viewport size
  vec2 viewport_size;
  {
    uint32_t w, h;
    viewport_get_resolution(viewport, &w, &h);
    glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
  }

  vec2 corners[4];
  rotate_tool_calc_selection_corners(camera, corners);

  // Draw a dashed rectangle (visible when multiple blocks are selected simultaneously)
  viewport_gizmo_draw_rect(corners, viewport_size, LineDrawMode_Dashed, (vec3){ 1.0f, 0.0f, 0.0f });

  // Draw the control points
  {
    vec2 control_points[4];
    rotate_tool_corners_to_control_points_clip(viewport_size, corners, control_points);
    viewport_gizmo_draw_control_points(control_points, 4);
  }
}