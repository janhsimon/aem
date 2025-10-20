#include "player.h"

#include "camera.h"
#include "collision.h"
#include "input.h"
#include "view_model.h"

#include <cglm/mat3.h>
#include <cglm/vec3.h>

#define GRAVITY 10.0f

#define PLAYER_RADIUS 0.25f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_ACCEL 0.3f
#define PLAYER_DECEL 0.1f
#define PLAYER_MOVE_SPEED 0.025f

static vec3 player_velocity = GLM_VEC3_ZERO_INIT;

void player_update(float delta_time,
                   float* level_vertices,
                   uint32_t* level_indices,
                   uint32_t level_index_count,
                   bool* moving)
{
  // Movement
  vec3 old_cam_pos;
  {
    cam_get_position(old_cam_pos);

    vec3 move;
    get_move_vector(move, moving);

    if (*moving)
    {
      mat3 cam_orientation;
      cam_get_orientation((float*)cam_orientation);

      glm_mat3_mulv(cam_orientation, move, move); // Transform move from local to camera space
      move[1] = 0.0f;                             // Flatten
      glm_vec3_normalize(move);
      glm_vec3_scale(move, PLAYER_ACCEL, move);

      glm_vec3_scale(move, delta_time, move);

      glm_vec3_add(move, player_velocity, player_velocity);
    }
    else
    {
      glm_vec3_scale(player_velocity, 1.0f - PLAYER_DECEL, player_velocity);
    }

    // Limit max speed
    {
      const float orig_y = player_velocity[1];
      player_velocity[1] = 0.0f;
      if (glm_vec3_norm(player_velocity) > PLAYER_MOVE_SPEED)
      {
        glm_vec3_normalize(player_velocity);
        glm_vec3_scale(player_velocity, PLAYER_MOVE_SPEED, player_velocity);
      }
      player_velocity[1] = orig_y;
    }

    player_velocity[1] = -GRAVITY * delta_time; // Gravity

    camera_add_move(player_velocity);
  }

  // New-style collision
  {
    vec3 new_cam_pos;
    cam_get_position(new_cam_pos);

    vec3 new_bottom;
    new_bottom[0] = new_cam_pos[0];
    new_bottom[1] = new_cam_pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS;
    new_bottom[2] = new_cam_pos[2];

    const float v = glm_vec3_norm(player_velocity);
    const int step_count = (v / PLAYER_RADIUS) + 1;
    const float step_length = v / step_count;
    for (int step = 1; step <= step_count; ++step)
    {
      vec3 t;
      glm_vec3_scale_as(player_velocity, step_length * step, t);

      vec3 step_cam_pos;
      glm_vec3_add(old_cam_pos, t, step_cam_pos);

      vec3 step_base;
      step_base[0] = step_cam_pos[0];
      step_base[1] = step_cam_pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS + PLAYER_RADIUS;
      step_base[2] = step_cam_pos[2];

      if (collide_capsule(step_base, step_cam_pos, PLAYER_RADIUS, level_vertices, level_indices, level_index_count))
      {
        cam_set_position(step_cam_pos);
        break;
      }
    }
  }

  // Mouse look
  double delta_x, delta_y;
  get_mouse_delta(&delta_x, &delta_y);
  camera_add_yaw_pitch(delta_x * 0.001f, delta_y * 0.001f);
}