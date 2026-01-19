#include "player.h"

#include "camera.h"
#include "collision.h"
#include "hud/hud_damage_indicator.h"
#include "input.h"
#include "map.h"
#include "preferences.h"
#include "sound.h"
#include "view_model.h"

#include <cglm/mat3.h>
#include <cglm/vec3.h>

#define GRAVITY 40.0f

#define PLAYER_RADIUS 0.25f
#define PLAYER_HEIGHT 1.8f

#define PLAYER_ACCEL 20.0f
#define PLAYER_DECEL 20.0f
#define PLAYER_MOVE_SPEED 5.0f

#define PLAYER_JUMP_STRENGTH 10.0f

#define PLAYER_MIN_RESPAWN_COOLDOWN 3.0f // How long to wait after death until the player can respawn, in seconds

static vec3 player_velocity = GLM_VEC3_ZERO_INIT;
static bool player_grounded = false;

static float health = 100.0f;

static float death_feet_height = 0.0f;

static bool has_fire_key_been_up_since_death;
static float respawn_cooldown = 0.0f;

void player_update(const struct Preferences* preferences, bool mouse_look, float delta_time, bool* moving)
{
  // Death animation
  if (health <= 0.0f)
  {
    // Let the camera drop to the feet
    {
      vec3 pos;
      cam_get_position(pos);
      if (death_feet_height < pos[1])
      {
        pos[1] -= delta_time * 3.0f;
        if (pos[1] < death_feet_height)
        {
          pos[1] = death_feet_height;
        }
        cam_set_position(pos);
      }
    }

    // And roll 90 deg to the side
    {
      camera_add_yaw_pitch_roll(0.0f, 0.0f, delta_time * GLM_PIf);
      float yaw, pitch, roll;
      camera_get_yaw_pitch_roll(&yaw, &pitch, &roll);
      if (roll > GLM_PI_2f)
      {
        roll = GLM_PI_2f;
        camera_set_yaw_pitch_roll(yaw, pitch, roll);
      }
    }

    // Respawn
    {
      const bool fire_key_down = get_shoot_button_down();
      if (!fire_key_down)
      {
        has_fire_key_been_up_since_death = true;
      }

      if (respawn_cooldown < PLAYER_MIN_RESPAWN_COOLDOWN)
      {
        respawn_cooldown += delta_time;
        if (respawn_cooldown > PLAYER_MIN_RESPAWN_COOLDOWN)
        {
          respawn_cooldown = PLAYER_MIN_RESPAWN_COOLDOWN;
        }
      }

      if (fire_key_down && respawn_cooldown >= PLAYER_MIN_RESPAWN_COOLDOWN && has_fire_key_been_up_since_death)
      {
        health = 100.0f;

        vec3 spawn_position;
        float spawn_yaw;
        get_current_map_random_enemy_spawn(spawn_position, &spawn_yaw);
        spawn_position[1] += PLAYER_HEIGHT - PLAYER_RADIUS;
        cam_set_position(spawn_position);
        camera_set_yaw_pitch_roll(spawn_yaw, 0.0f, 0.0f);

        view_model_respawn();

        respawn_cooldown = 0.0f;
      }
    }
  }
  else
  {
    // Movement
    vec3 start_cam_pos;
    {
      cam_get_position(start_cam_pos);

      vec3 move;
      get_move_vector(move, moving);

      if (*moving)
      {
        mat3 cam_rotation;
        cam_calc_rotation(cam_rotation, CameraRotationMode_WithoutRecoil);
        glm_mat3_mulv(cam_rotation, move, move); // Transform move from local to camera space

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

        if (!preferences->no_clip)
        {
          player_velocity[1] = original_y;
        }
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
      camera_add_yaw_pitch_roll(delta_x * 0.001f, delta_y * 0.001f, 0.0f);
    }
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

float calc_angle_delta_towards_player(vec3 from_position, vec3 from_forward)
{
  const float current_yaw = atan2f(from_forward[0], from_forward[2]);

  float target_yaw = 0.0f;
  {
    vec3 player_position;
    cam_get_position(player_position);

    vec3 dir;
    glm_vec3_sub(player_position, from_position, dir);

    // Ignore vertical difference
    dir[1] = 0.0f;

    // Prevent division by zero
    if (glm_vec3_norm2(dir) > 0.0001f)
    {
      glm_vec3_normalize(dir);
      target_yaw = atan2f(dir[0], dir[2]);
    }
  }

  float delta = target_yaw - current_yaw;
  while (delta > GLM_PI)
  {
    delta -= GLM_PI * 2.0f;
  }

  while (delta < -GLM_PI)
  {
    delta += GLM_PI * 2.0f;
  }

  return glm_deg(delta);
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

float get_player_health()
{
  return health;
}

bool is_player_hit(vec3 from, vec3 to)
{
  vec3 player_top;
  cam_get_position(player_top);

  vec3 player_bottom;
  glm_vec3_copy(player_top, player_bottom);
  player_bottom[1] -= PLAYER_HEIGHT - PLAYER_RADIUS * 2.0f;

  vec3 a, b;
  closest_segment_segment(from, to, player_top, player_bottom, a, b);
  glm_vec3_sub(a, b, a);
  const float dist = glm_vec3_norm(a);
  return dist < (PLAYER_RADIUS * 0.33f);
}

void player_hurt(float damage, vec3 dir)
{
  if (health <= 0.0f)
  {
    return;
  }

  // TODO: Could also do this with a separate 'layer' of camera 'recoil' to do custom easing back to the origin
  const float visual_damage = min(damage, 25.0f);
  camera_add_recoil_yaw_pitch(((((rand() % 100) / 100.0f) * visual_damage) - visual_damage * 0.5f) * 0.01f,
                              ((((rand() % 100) / 100.0f) * visual_damage) - visual_damage * 0.5f) * 0.01f);

  play_player_hurt_sound();

  health -= damage;

  if (health < 0)
  {
    // Die
    health = 0;

    vec3 cam_pos;
    cam_get_position(cam_pos);
    death_feet_height = cam_pos[1] - PLAYER_HEIGHT + (PLAYER_RADIUS + 0.1f);

    has_fire_key_been_up_since_death = false;
  }

  // Pain indicator
  {
    hud_damage_indicate(dir);
  }
}

float player_get_min_respawn_cooldown()
{
  return PLAYER_MIN_RESPAWN_COOLDOWN;
}

float player_get_respawn_cooldown()
{
  return respawn_cooldown;
}