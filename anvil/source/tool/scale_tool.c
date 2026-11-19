#include "scale_tool.h"

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
#define CONTROL_POINT_BOTTOM 4
#define CONTROL_POINT_LEFT 5
#define CONTROL_POINT_TOP 6
#define CONTROL_POINT_RIGHT 7

static bool is_scaling;
static uint32_t scale_control_point_index;

static void calc_selection_corners_clip(struct Camera* camera, vec2 out_selection_corners[4])
{
  vec3 bbox_min_world, bbox_max_world;
  map_get_selection_bbox_world(bbox_min_world, bbox_max_world);

  vec2 bbox_min_clip, bbox_max_clip;
  camera_world_to_clip(camera, bbox_min_world, bbox_min_clip);
  camera_world_to_clip(camera, bbox_max_world, bbox_max_clip);

  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_min_clip[1] }, out_selection_corners[0]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_min_clip[1] }, out_selection_corners[1]);
  glm_vec2_copy((vec2){ bbox_max_clip[0], bbox_max_clip[1] }, out_selection_corners[2]);
  glm_vec2_copy((vec2){ bbox_min_clip[0], bbox_max_clip[1] }, out_selection_corners[3]);
}

static void calc_control_points_clip(struct Camera* camera, vec2 viewport_size, vec2 out_control_points[8])
{
  vec2 corners[4];
  calc_selection_corners_clip(camera, corners);

  // Determine center between corners
  vec2 center = GLM_VEC2_ZERO_INIT;
  {
    for (uint32_t corner_index = 0; corner_index < 4; ++corner_index)
    {
      glm_vec2_add(center, corners[corner_index], center);
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
    glm_vec2_copy(corners[corner_index], out_control_points[corner_index]);

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

static void calc_control_points_screen(struct Viewport* viewport, vec2 out_control_points[8])
{
  vec2 viewport_pos, viewport_size;
  {
    uint32_t x, y, w, h;
    viewport_get_position(viewport, &x, &y);
    viewport_get_resolution(viewport, &w, &h);
    glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
    glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
  }

  calc_control_points_clip(viewport_get_camera(viewport), viewport_size, out_control_points);

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
  if (button != MouseButton_Left)
  {
    return false;
  }

  is_scaling = false;

  // Check control points
  if (map_has_selection())
  {
    vec2 control_points[8];
    calc_control_points_screen(viewport, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < CONTROL_POINT_SELECTION_RADIUS)
      {
        is_scaling = true;
        scale_control_point_index = control_point_index;
        return true;
      }
    }
  }

  return false;
}

bool scale_tool_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Check for control points
  {
    vec2 control_points[8];
    calc_control_points_screen(viewport, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
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

bool scale_tool_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  // Check for corner control points
  if (map_has_selection())
  {
    vec2 control_points[8];
    calc_control_points_screen(viewport, control_points);

    for (uint32_t control_point_index = 0; control_point_index < 8; ++control_point_index)
    {
      vec2 v;
      glm_vec2_sub(mouse_position, control_points[control_point_index], v);

      const float dist = glm_vec2_norm(v);
      if (dist < CONTROL_POINT_SELECTION_RADIUS)
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

  return false;
}

bool scale_tool_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left || !is_scaling)
  {
    return false;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  // Snapping
  grid_snap(mouse_pos_world, mouse_pos_world);

  vec3 bbox_min_world, bbox_max_world;
  map_get_selection_bbox_world(bbox_min_world, bbox_max_world);

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

  const float min_grid_size = get_min_grid_size();

  // Left
  if (scale_control_point_index == CONTROL_POINT_LEFT || scale_control_point_index == CONTROL_POINT_BOTTOM_LEFT ||
      scale_control_point_index == CONTROL_POINT_TOP_LEFT)
  {
    bbox_max_world[axis_index_horizontal] = mouse_pos_world[axis_index_horizontal];

    if (bbox_max_world[axis_index_horizontal] < bbox_min_world[axis_index_horizontal] + min_grid_size)
    {
      bbox_max_world[axis_index_horizontal] = bbox_min_world[axis_index_horizontal] + min_grid_size;
    }
  }

  // Right
  if (scale_control_point_index == CONTROL_POINT_RIGHT || scale_control_point_index == CONTROL_POINT_BOTTOM_RIGHT ||
      scale_control_point_index == CONTROL_POINT_TOP_RIGHT)
  {
    bbox_min_world[axis_index_horizontal] = mouse_pos_world[axis_index_horizontal];

    if (bbox_min_world[axis_index_horizontal] > bbox_max_world[axis_index_horizontal] - min_grid_size)
    {
      bbox_min_world[axis_index_horizontal] = bbox_max_world[axis_index_horizontal] - min_grid_size;
    }
  }

  // Top
  if (scale_control_point_index == CONTROL_POINT_TOP || scale_control_point_index == CONTROL_POINT_TOP_LEFT ||
      scale_control_point_index == CONTROL_POINT_TOP_RIGHT)
  {
    bbox_max_world[axis_index_vertical] = mouse_pos_world[axis_index_vertical];

    if (bbox_max_world[axis_index_vertical] < bbox_min_world[axis_index_vertical] + min_grid_size)
    {
      bbox_max_world[axis_index_vertical] = bbox_min_world[axis_index_vertical] + min_grid_size;
    }
  }

  // Bottom
  if (scale_control_point_index == CONTROL_POINT_BOTTOM || scale_control_point_index == CONTROL_POINT_BOTTOM_LEFT ||
      scale_control_point_index == CONTROL_POINT_BOTTOM_RIGHT)
  {
    bbox_min_world[axis_index_vertical] = mouse_pos_world[axis_index_vertical];

    if (bbox_min_world[axis_index_vertical] > bbox_max_world[axis_index_vertical] - min_grid_size)
    {
      bbox_min_world[axis_index_vertical] = bbox_max_world[axis_index_vertical] - min_grid_size;
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

  map_scale_selection(bbox_min_world, bbox_max_world);
  return true;
}

void scale_tool_draw_gizmo(const struct Viewport* viewport)
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

  // Draw a dashed rectangle (visible when multiple blocks are selected simultaneously)
  {
    vec2 corners[4];
    calc_selection_corners_clip(camera, corners);
    viewport_gizmo_draw_rect(corners, viewport_size, LineDrawMode_Dashed, (vec3){ 1.0f, 0.0f, 0.0f });
  }

  // Draw the control points
  {
    vec2 control_points[8];
    calc_control_points_clip(viewport_get_camera(viewport), viewport_size, control_points);
    viewport_gizmo_draw_control_points(control_points, 8);
  }
}