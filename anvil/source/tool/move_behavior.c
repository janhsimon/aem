#include "move_behavior.h"

#include "grid.h"
#include "input.h"
#include "map/map.h"
#include "viewport/camera.h"
#include "viewport/viewport.h"

#include <cglm/vec3.h>

#define SELECTION_RADIUS 10.0f

static bool is_moving, is_moving_first_frame;
static vec3 move_delta;

bool move_behavior_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left)
  {
    return false;
  }

  // Start a move
  {
    struct Camera* camera = viewport_get_camera(viewport);

    vec3 mouse_pos_world;
    camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

    vec3 anchor;
    map_get_selection_first_vertex(anchor);
    glm_vec3_sub(anchor, mouse_pos_world, move_delta);

    is_moving = is_moving_first_frame = true;
  }

  return true;
}

void move_behavior_on_mouse_button_clicked()
{
  is_moving = false;
}

bool move_behavior_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (button != MouseButton_Left || !is_moving)
  {
    return false;
  }

  if (is_moving_first_frame)
  {
    if (input_is_duplicate_key_down())
    {
      map_duplicate_selection();
    }

    is_moving_first_frame = false;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  vec3 m;
  glm_vec3_add(mouse_pos_world, move_delta, m);

  // Snap to grid
  grid_snap(m, m);
  map_move_selection(m);

  return true;
}

void move_behavior_on_mouse_drag_finished()
{
  is_moving = false;
}

void move_behavior_reanchor(struct Viewport* viewport, vec2 mouse_position)
{
  if (!is_moving)
  {
    return;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec3 mouse_pos_world;
  camera_screen_to_world_2d(camera, mouse_position, mouse_pos_world);

  vec3 anchor;
  map_get_selection_first_vertex(anchor);
  glm_vec3_sub(anchor, mouse_pos_world, move_delta);
}