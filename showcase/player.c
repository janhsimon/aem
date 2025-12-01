#include "player.h"

#include "camera.h"
#include "collision.h"
#include "input.h"
#include "preferences.h"

#include <cglm/mat3.h>
#include <cglm/vec3.h>

#define GRAVITY 100.0f

#define PLAYER_RADIUS 0.25f
#define PLAYER_HEIGHT 1.8f

#define PLAYER_ACCEL 20.0f
#define PLAYER_DECEL 20.0f
#define PLAYER_MOVE_SPEED 5.0f

static vec3 player_velocity = GLM_VEC3_ZERO_INIT;

void player_update(const struct Preferences* preferences, bool mouse_look, float delta_time, bool* moving)
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
      cam_get_orientation(cam_orientation);

      glm_mat3_mulv(cam_orientation, move, move); // Transform move from local to camera space

      if (!preferences->no_clip)
      {
        move[1] = 0.0f; // Flatten
      }

      glm_vec3_normalize(move);
      glm_vec3_scale(move, PLAYER_ACCEL * delta_time, move);
      glm_vec3_add(move, player_velocity, player_velocity);
    }
    else
    {
      glm_vec3_scale(player_velocity, 1.0f - (PLAYER_DECEL * delta_time), player_velocity);
    }

    // Limit max speed
    {
      const float orig_y = player_velocity[1];

      if (!preferences->no_clip)
      {
        player_velocity[1] = 0.0f;
      }

      if (glm_vec3_norm(player_velocity) > PLAYER_MOVE_SPEED * delta_time)
      {
        glm_vec3_scale_as(player_velocity, PLAYER_MOVE_SPEED * delta_time, player_velocity);
      }

      if (!preferences->no_clip)
      {
        player_velocity[1] = orig_y;
      }
    }
  }

  // Collision
  if (!preferences->no_clip)
  {
    vec3 start_pos;
    cam_get_position(start_pos);

    // Phase 1: Horizontal movement versus walls
    vec3 pos;
    {
      vec3 horizontal_move = { player_velocity[0], 0.0f, player_velocity[2] };
      glm_vec3_add(start_pos, horizontal_move, pos);

      // Capsule endpoints for collision
      vec3 base = { pos[0], pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS * 2, pos[2] };
      vec3 top = { pos[0], pos[1], pos[2] };

      // Resolve walls only, no floors
      collide_capsule(base, top, PLAYER_RADIUS, CollisionPhase_Walls);
      glm_vec3_copy(top, pos); // Update pos from capsule end
    }

    // Phase 2: Vertical movement versus floors
    {
      float vy = player_velocity[1];
      vy -= GRAVITY * delta_time;

      vec3 vertical_move = { 0.0f, vy * delta_time, 0.0f };
      glm_vec3_add(pos, vertical_move, pos);

      // Capsule endpoints for collision
      vec3 base = { pos[0], pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS * 2, pos[2] };
      vec3 top = { pos[0], pos[1], pos[2] };

      const bool grounded = collide_capsule(base, top, PLAYER_RADIUS, CollisionPhase_Floors);
      glm_vec3_copy(top, pos); // Update final pos

      // Ground behavior
      if (grounded && vy < 0.0f)
      {
        vy = 0.0f;
      }

      player_velocity[1] = vy;
    }

    cam_set_position(pos);
  }
  else
  {
    glm_vec3_add(start_cam_pos, player_velocity, start_cam_pos);
    cam_set_position(start_cam_pos);
  }

  // Mouse look
  if (mouse_look)
  {
    double delta_x, delta_y;
    get_mouse_delta(&delta_x, &delta_y);
    camera_add_yaw_pitch(delta_x * 0.001f, delta_y * 0.001f);
  }
}

float get_player_speed()
{
  vec3 flat_speed;
  glm_vec3_copy(player_velocity, flat_speed);
  flat_speed[1] = 0.0f;
  return glm_vec3_norm(flat_speed);
}