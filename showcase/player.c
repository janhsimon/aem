#include "player.h"

#include "camera.h"
#include "collision.h"
#include "input.h"
#include "preferences.h"

#include <cglm/mat3.h>
#include <cglm/vec3.h>

#define GRAVITY 40.0f

#define PLAYER_RADIUS 0.25f
#define PLAYER_HEIGHT 1.8f

#define PLAYER_ACCEL 20.0f
#define PLAYER_DECEL 20.0f
#define PLAYER_MOVE_SPEED 5.0f

#define PLAYER_JUMP_STRENGTH 10.0f

static vec3 player_velocity = GLM_VEC3_ZERO_INIT;
static bool player_grounded = false;

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
      const float original_y = player_velocity[1];
      glm_vec3_scale(player_velocity, 1.0f - (PLAYER_DECEL * delta_time), player_velocity);
      player_velocity[1] = original_y;
    }

    // Limit max speed
    {
      const float orig_y = player_velocity[1];

      if (!preferences->no_clip)
      {
        player_velocity[1] = 0.0f;
      }

      float speed_limit = PLAYER_MOVE_SPEED * delta_time;
      if (preferences->no_clip)
      {
        speed_limit *= 2.0f; // Go faster in noclip
      }

      if (glm_vec3_norm(player_velocity) > speed_limit)
      {
        glm_vec3_scale_as(player_velocity, speed_limit, player_velocity);
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
    // Phase 1: Horizontal movement versus walls
    vec3 pos;
    {
      cam_get_position(pos);

      vec3 horizontal_move = { player_velocity[0], 0.0f, player_velocity[2] };

      const float magnitude = glm_vec3_norm(horizontal_move);
      const int step_count = (magnitude / PLAYER_RADIUS) + 1;
      const float step_length = magnitude / step_count;

      vec3 step_vector;
      glm_vec3_scale_as(horizontal_move, step_length, step_vector);

      for (int step = 0; step < step_count; ++step)
      {
        glm_vec3_add(pos, step_vector, pos);
        vec3 base = { pos[0], pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS * 2, pos[2] };
        collide_capsule(base, pos, PLAYER_RADIUS, CollisionPhase_Walls); // Resolve walls only, no floors
      }
    }

    // Phase 2: Vertical movement versus floors
    {
      float vy = player_velocity[1];

      if (player_grounded && get_jump_key_down())
      {
        vy = PLAYER_JUMP_STRENGTH;
      }

      vy -= GRAVITY * delta_time;

      vec3 vertical_move = { 0.0f, vy * delta_time, 0.0f };

      const float magnitude = glm_vec3_norm(vertical_move);
      const int step_count = (magnitude / PLAYER_RADIUS) + 1;
      const float step_length = magnitude / step_count;

      vec3 step_vector;
      glm_vec3_scale_as(vertical_move, step_length, step_vector);

      for (int step = 0; step < step_count; ++step)
      {
        glm_vec3_add(pos, step_vector, pos);
        vec3 base = { pos[0], pos[1] - PLAYER_HEIGHT + PLAYER_RADIUS * 2, pos[2] };
        player_grounded =
          collide_capsule(base, pos, PLAYER_RADIUS, CollisionPhase_Floors); // Resolve floors only, no walls

        // Ground behavior
        if (player_grounded)
        {
          if (vy < 0.0f)
          {
            vy = 0.0f;
          }

          break;
        }
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

void get_player_position(vec3 position)
{
  cam_get_position(position);
  position[1] -= PLAYER_HEIGHT - PLAYER_RADIUS;
}

void get_player_velocity(vec3 velocity)
{
  glm_vec3_copy(player_velocity, velocity);
}

bool get_player_grounded()
{
  return player_grounded;
}

void player_jump()
{
  if (!player_grounded)
  {
    return;
  }

  player_velocity[1] = 10.0f;
}