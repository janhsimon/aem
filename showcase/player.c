#include "player.h"

#include "camera.h"
#include "collision.h"
#include "input.h"

#include <cglm/mat3.h>
#include <cglm/vec3.h>

#define GRAVITY 10.0f

#define PLAYER_RADIUS 0.25f
#define PLAYER_HEIGHT 1.8f
#define PLAYER_ACCEL 0.3f
#define PLAYER_DECEL 0.1f
#define PLAYER_MOVE_SPEED 5.0f

static vec3 player_velocity = GLM_VEC3_ZERO_INIT;

void player_update(float delta_time, bool* moving)
{
  // Movement
  vec3 start_cam_pos;
  {
    cam_get_position(start_cam_pos);

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
      if (glm_vec3_norm(player_velocity) > PLAYER_MOVE_SPEED * delta_time)
      {
        glm_vec3_normalize(player_velocity);
        glm_vec3_scale(player_velocity, PLAYER_MOVE_SPEED * delta_time, player_velocity);
      }
      player_velocity[1] = orig_y;
    }

    player_velocity[1] = -GRAVITY * delta_time; // Gravity
  }

  // New-style collision
  {
    const float magnitude = glm_vec3_norm(player_velocity);
    const int step_count = (magnitude / PLAYER_RADIUS) + 1;
    const float step_length = magnitude / step_count;

    vec3 step_vector;
    glm_vec3_scale_as(player_velocity, step_length, step_vector);

    for (int step = 0; step < step_count; ++step)
    {
      glm_vec3_add(start_cam_pos, step_vector, start_cam_pos);

      vec3 start_base;
      start_base[0] = start_cam_pos[0];
      start_base[1] = start_cam_pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS + PLAYER_RADIUS;
      start_base[2] = start_cam_pos[2];

      collide_capsule(start_base, start_cam_pos, PLAYER_RADIUS);
    }

    cam_set_position(start_cam_pos);
  }

  // Mouse look
  double delta_x, delta_y;
  get_mouse_delta(&delta_x, &delta_y);
  camera_add_yaw_pitch(delta_x * 0.001f, delta_y * 0.001f);
}