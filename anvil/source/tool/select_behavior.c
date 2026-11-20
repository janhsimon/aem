#include "select_behavior.h"

#include "grid.h"
#include "input.h"
#include "map/map.h"
#include "viewport/camera.h"
#include "viewport/gizmo.h"
#include "viewport/viewport.h"

#include <cglm/mat4.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#define SELECTION_RADIUS 10.0f

#define NEAR_INFINITY 32768.0f

static struct Viewport* box_selection_viewport = NULL; // The viewport of the active box selection, if any
static vec3 box_selection_from, box_selection_to;      // Bounding box of selection rectangle in world space

bool select_behavior_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  uint32_t closest_block_index;
  float distance;

  if (viewport_get_type(viewport) == ViewportType_Perspective)
  {
    vec3 ray_origin, ray_direction;
    camera_calc_pick_ray(viewport_get_camera(viewport), mouse_position, ray_origin, ray_direction);

    if (map_pick_block_at(ray_origin, ray_direction, &closest_block_index, &distance))
    {
      if (!input_is_add_to_selection_key_down())
      {
        map_deselect_all_blocks();
        map_select_block(closest_block_index);
      }
      else
      {
        if (map_is_block_selected(closest_block_index))
        {
          map_deselect_block(closest_block_index);
        }
        else
        {
          map_select_block(closest_block_index);
        }
      }
    }
    else
    {
      if (!input_is_add_to_selection_key_down())
      {
        map_deselect_all_blocks();
      }
    }

    return true;
  }

  // Check if the mouse was clicked inside the selection bbox
  if (map_is_mouse_in_selection_bbox(viewport, mouse_position))
  {
    // Check if an unselected block is close
    if (input_is_add_to_selection_key_down())
    {
      const bool block_found =
        map_get_closest_block_to(viewport, mouse_position, MapGetClosestBlockSelectionFilter_All,
                                 MapGetClosestBlockAreaFilter_All, &closest_block_index, &distance);
      if (block_found && distance <= SELECTION_RADIUS)
      {
        if (map_is_block_selected(closest_block_index))
        {
          map_deselect_block(closest_block_index);
          if (!map_has_selection())
          {
            // Consume the event if the last selected block was just deselected
            return true;
          }
        }
        else
        {
          map_select_block(closest_block_index);
        }
      }
    }

    // Do not consume the event to allow the move behavior to initiate a move right away
    return false;
  }

  // The mouse was clicked outside the selection bbox (if one exists)
  // Now find out if the mouse was clicked close to a block
  // Prioritize selected blocks, then center crosses of unselected, and then edges of unselected blocks
  bool block_found = map_get_closest_block_to(viewport, mouse_position, MapGetClosestBlockSelectionFilter_OnlySelected,
                                              MapGetClosestBlockAreaFilter_All, &closest_block_index, &distance);
  if (!block_found || distance > SELECTION_RADIUS)
  {
    block_found = map_get_closest_block_to(viewport, mouse_position, MapGetClosestBlockSelectionFilter_OnlyUnselected,
                                           MapGetClosestBlockAreaFilter_OnlyCenter, &closest_block_index, &distance);
    if (!block_found || distance > SELECTION_RADIUS)
    {
      block_found = map_get_closest_block_to(viewport, mouse_position, MapGetClosestBlockSelectionFilter_OnlyUnselected,
                                             MapGetClosestBlockAreaFilter_OnlyEdges, &closest_block_index, &distance);
    }
  }

  // If a block was found that close to the mouse click
  if (block_found && distance <= SELECTION_RADIUS)
  {
    // Select the close block if it was not already selected
    if (!map_is_block_selected(closest_block_index))
    {
      if (!input_is_add_to_selection_key_down())
      {
        map_deselect_all_blocks();
      }

      map_select_block(closest_block_index);

      // Do not consume the event to allow the move behavior to initiate a move right away
      return false;
    }

    // The close block was already selected
    // Note that this only happens if the close block is on the edge of the selection bbox and the user clicks just
    // outside of that box but close enough to the block
    else
    {
      if (input_is_add_to_selection_key_down())
      {
        map_deselect_block(closest_block_index);

        // Consume the event to not allow a move to be initiated right away
        return true;
      }
      else
      {
        // Do not consume the event to allow the move behavior to initiate a move right away
        return false;
      }
    }
  }

  // If the mouse click was not close to anything

  // Deselect everything
  if (!input_is_add_to_selection_key_down())
  {
    map_deselect_all_blocks();
  }

  // Initiate a bounding box selection
  {
    // Make initial selection bbox
    struct Camera* camera = viewport_get_camera(viewport);

    camera_screen_to_world_2d(camera, mouse_position, box_selection_from);
    grid_snap(box_selection_from, box_selection_from); // Snap to grid
    glm_vec3_copy(box_selection_from, box_selection_to);

    // Extend depth-component of volume to near-infinity
    {
      vec3 camera_forward;
      camera_get_forward(camera, camera_forward);
      for (int i = 0; i < 3; ++i)
      {
        if (camera_forward[i] != 0.0f)
        {
          box_selection_from[i] = -NEAR_INFINITY;
          box_selection_to[i] = NEAR_INFINITY;
        }
      }
    }

    box_selection_viewport = viewport;
  }

  // Consume the event
  return true;
}

bool select_behavior_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  if (!map_is_mouse_in_selection_bbox(viewport, mouse_position))
  {
    float distance;
    uint32_t closest_block_index;
    if (!map_get_closest_block_to(viewport, mouse_position, MapGetClosestBlockSelectionFilter_All,
                                  MapGetClosestBlockAreaFilter_All, &closest_block_index, &distance))
    {
      return false;
    }

    if (distance > SELECTION_RADIUS)
    {
      return false;
    }
  }

  input_use_move_cursor();
  return true;
}

bool select_behavior_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Extend selection bbox
  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  // Only override the correct axes
  {
    vec3 camera_forward;
    camera_get_forward(camera, camera_forward);
    for (int i = 0; i < 3; ++i)
    {
      if (camera_forward[i] == 0.0f)
      {
        box_selection_to[i] = mouse_pos_world[i];
      }
    }
  }

  grid_snap(box_selection_to, box_selection_to);

  input_use_draw_cursor();
  return true;
}

static void get_bbox(vec3 min, vec3 max)
{
  // Fix the bounding box
  for (int i = 0; i < 3; ++i)
  {
    if (box_selection_from[i] < box_selection_to[i])
    {
      min[i] = box_selection_from[i];
      max[i] = box_selection_to[i];
    }
    else
    {
      min[i] = box_selection_to[i];
      max[i] = box_selection_from[i];
    }
  }
}

void select_behavior_on_mouse_drag_finished()
{
  if (!box_selection_viewport)
  {
    return;
  }

  vec3 bbox_min, bbox_max;
  get_bbox(bbox_min, bbox_max);
  map_select_blocks_in_bbox(bbox_min, bbox_max);

  box_selection_viewport = NULL;
}

void select_behavior_draw_gizmo(const struct Viewport* viewport)
{
  // Draw the wireframe overlay in perspective (ie. 3D) viewports
  if (viewport_get_type(viewport) == ViewportType_Perspective)
  {
    if (map_has_selection())
    {
      struct Camera* camera = viewport_get_camera(viewport);

      mat4 view, proj, viewproj;
      camera_calc_view_matrix(camera, view);
      camera_calc_proj_matrix(camera, proj);
      glm_mat4_mul(proj, view, viewproj);

      const uint32_t block_count = map_get_block_count();
      for (uint32_t block_index = 0; block_index < block_count; ++block_index)
      {
        if (!map_is_block_selected(block_index))
        {
          continue;
        }

        vec3 from, to;
        map_get_block_bbox(block_index, from, to);

        viewport_gizmo_draw_wireframe_cube(from, to, (vec3){ 1.0f, 1.0f, 0.0f }, viewproj);
      }
    }
  }
  // Draw the selection rectangle in non-perspective (ie. 2D) viewports
  else if (box_selection_viewport == viewport)
  {
    // Get the selection corners in clip space
    vec2 corners[4];
    {
      struct Camera* camera = viewport_get_camera(viewport);

      vec2 bbox_from_clip, bbox_to_clip;
      camera_world_to_clip(camera, box_selection_from, bbox_from_clip);
      camera_world_to_clip(camera, box_selection_to, bbox_to_clip);

      glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_from_clip[1] }, corners[0]);
      glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_from_clip[1] }, corners[1]);
      glm_vec2_copy((vec2){ bbox_to_clip[0], bbox_to_clip[1] }, corners[2]);
      glm_vec2_copy((vec2){ bbox_from_clip[0], bbox_to_clip[1] }, corners[3]);
    }

    // Get the viewport size
    vec2 viewport_size;
    {
      uint32_t w, h;
      viewport_get_resolution(viewport, &w, &h);
      glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
    }

    // Draw a dashed selection rectangle
    viewport_gizmo_draw_rect(corners, viewport_size, LineDrawMode_Dashed, (vec3){ 1.0f, 1.0f, 0.0f });
  }
}