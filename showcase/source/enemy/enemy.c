#include "enemy.h"

#include "camera.h"
#include "collision.h"
#include "debug_manager.h"
#include "enemy_state.h"
#include "enemy_state_aim.h"
#include "enemy_state_die.h"
#include "enemy_state_fire.h"
#include "enemy_state_flinch.h"
#include "enemy_state_walk.h"
#include "map.h"
#include "model_manager.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/vec2.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define ENEMY_COLLIDER_RADIUS 0.5f
#define ENEMY_COLLIDER_HEIGHT 1.8f

#define ENEMY_HITBOX_HEAD_JOINT_INDEX 16
#define ENEMY_HITBOX_HEAD_RADIUS 0.11f
#define ENEMY_HITBOX_HEAD_X 0.0f
#define ENEMY_HITBOX_HEAD_BOTTOM_Y 0.3f
#define ENEMY_HITBOX_HEAD_TOP_Y 0.7f
#define ENEMY_HITBOX_HEAD_Z 0.2f

#define ENEMY_HITBOX_UPPER_TORSO_JOINT_INDEX 14
#define ENEMY_HITBOX_UPPER_TORSO_RADIUS 0.25f
#define ENEMY_HITBOX_UPPER_TORSO_X 0.0f
#define ENEMY_HITBOX_UPPER_TORSO_BOTTOM_Y 0.2f
#define ENEMY_HITBOX_UPPER_TORSO_TOP_Y 0.35f
#define ENEMY_HITBOX_UPPER_TORSO_Z 0.0f

#define ENEMY_HITBOX_LOWER_TORSO_JOINT_INDEX 2
#define ENEMY_HITBOX_LOWER_TORSO_RADIUS 0.2f
#define ENEMY_HITBOX_LOWER_TORSO_X 0.0f
#define ENEMY_HITBOX_LOWER_TORSO_BOTTOM_Y -0.15f
#define ENEMY_HITBOX_LOWER_TORSO_TOP_Y 0.7f
#define ENEMY_HITBOX_LOWER_TORSO_Z 0.0f

static struct ModelRenderInfo* render_info = NULL;

static enum EnemyState state;

static const struct Preferences* preferences = NULL;

static GLuint joint_transform_buffer, joint_transform_texture;
static mat4* joint_transforms;

static struct AEMAnimationMixer* mixer;

static mat4 transform;

static vec3 hitbox_head_bottom, hitbox_head_top;
static vec3 hitbox_upper_torso_bottom, hitbox_upper_torso_top;
static vec3 hitbox_lower_torso_bottom, hitbox_lower_torso_top;

static float health;

static void respawn_enemy(bool play_sound)
{
  // Reset health
  health = 100.0f;

  // Reset position and angle
  vec3 spawn_position;
  float spawn_yaw;
  get_current_map_random_enemy_spawn(spawn_position, &spawn_yaw);
  glm_translate_make(transform, spawn_position);
  glm_rotate_y(transform, glm_rad(spawn_yaw + 90.0f),
               transform); // TODO: This is a hack because the enemy doesn't yet use angles
  glm_scale(transform, (vec3){ 19.25f, 19.25f, 19.25f });

  // Play the optional respawn sound effect
  if (play_sound)
  {
    play_respawn_sound(spawn_position);
  }

  enter_enemy_state_walk(true);
}

bool load_enemy(const struct Preferences* preferences_)
{
  preferences = preferences_;

  render_info = load_model("models/soldier.aem");
  if (!render_info)
  {
    return false;
  }

  glGenBuffers(1, &joint_transform_buffer);
  glGenTextures(1, &joint_transform_texture);

  const uint32_t joint_count = aem_get_model_joint_count(render_info->model);

  if (aem_load_animation_mixer(joint_count, 4, &mixer) != AEMAnimationMixerResult_Success)
  {
    return false;
  }

  aem_set_animation_mixer_enabled(mixer, true);
  aem_set_animation_mixer_blend_speed(mixer, 4.0f);

  load_enemy_state_walk(preferences, &state, render_info->model, mixer);
  load_enemy_state_aim(preferences, &state, mixer);
  load_enemy_state_fire(preferences, &state, render_info->model, mixer);
  load_enemy_state_flinch(&state, render_info->model, mixer);
  load_enemy_state_die(&state, render_info->model, mixer);

  // Enable skeletal animations
  {
    joint_transforms = malloc(joint_count * sizeof(*joint_transforms));
    assert(joint_transforms);

    for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
    {
      glm_mat4_identity(joint_transforms[joint_index]);
    }

    glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * joint_count, NULL, GL_DYNAMIC_DRAW);

    glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, joint_transform_buffer);
  }

  respawn_enemy(false);

  return true;
}

static void update_hitboxes()
{
  // Head
  {
    mat4 hitbox_head_transform;
    aem_get_animation_mixer_joint_transform(render_info->model, mixer, ENEMY_HITBOX_HEAD_JOINT_INDEX,
                                            (float*)hitbox_head_transform);
    glm_mat4_mul(transform, hitbox_head_transform, hitbox_head_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_HEAD_X, ENEMY_HITBOX_HEAD_BOTTOM_Y, ENEMY_HITBOX_HEAD_Z }, hitbox_head_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_HEAD_X, ENEMY_HITBOX_HEAD_TOP_Y, ENEMY_HITBOX_HEAD_Z }, hitbox_head_top);

    glm_mat4_mulv3(hitbox_head_transform, hitbox_head_bottom, 1.0f, hitbox_head_bottom);
    glm_mat4_mulv3(hitbox_head_transform, hitbox_head_top, 1.0f, hitbox_head_top);
  }

  // Upper torso
  {
    mat4 hitbox_upper_torso_transform;
    aem_get_animation_mixer_joint_transform(render_info->model, mixer, ENEMY_HITBOX_UPPER_TORSO_JOINT_INDEX,
                                            (float*)hitbox_upper_torso_transform);
    glm_mat4_mul(transform, hitbox_upper_torso_transform, hitbox_upper_torso_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_UPPER_TORSO_X, ENEMY_HITBOX_UPPER_TORSO_BOTTOM_Y, ENEMY_HITBOX_UPPER_TORSO_Z },
                  hitbox_upper_torso_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_UPPER_TORSO_X, ENEMY_HITBOX_UPPER_TORSO_TOP_Y, ENEMY_HITBOX_UPPER_TORSO_Z },
                  hitbox_upper_torso_top);

    glm_mat4_mulv3(hitbox_upper_torso_transform, hitbox_upper_torso_bottom, 1.0f, hitbox_upper_torso_bottom);
    glm_mat4_mulv3(hitbox_upper_torso_transform, hitbox_upper_torso_top, 1.0f, hitbox_upper_torso_top);
  }

  // Lower torso
  {
    mat4 hitbox_lower_torso_transform;
    aem_get_animation_mixer_joint_transform(render_info->model, mixer, ENEMY_HITBOX_LOWER_TORSO_JOINT_INDEX,
                                            (float*)hitbox_lower_torso_transform);
    glm_mat4_mul(transform, hitbox_lower_torso_transform, hitbox_lower_torso_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_LOWER_TORSO_X, ENEMY_HITBOX_LOWER_TORSO_BOTTOM_Y, ENEMY_HITBOX_LOWER_TORSO_Z },
                  hitbox_lower_torso_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_LOWER_TORSO_X, ENEMY_HITBOX_LOWER_TORSO_TOP_Y, ENEMY_HITBOX_LOWER_TORSO_Z },
                  hitbox_lower_torso_top);

    glm_mat4_mulv3(hitbox_lower_torso_transform, hitbox_lower_torso_bottom, 1.0f, hitbox_lower_torso_bottom);
    glm_mat4_mulv3(hitbox_lower_torso_transform, hitbox_lower_torso_top, 1.0f, hitbox_lower_torso_top);
  }
}

static bool calc_player_visible()
{
  // First test if the player is somewhat in front of the enemy
  {
    vec3 enemy_dir;
    glm_vec3_normalize_to(transform[2], enemy_dir);

    vec3 enemy_to_player;
    {
      vec3 player_pos;
      cam_get_position(player_pos);

      glm_vec3_sub(player_pos, transform[3], enemy_to_player);
      enemy_to_player[1] = 0.0f; // Flatten

      glm_vec3_normalize(enemy_to_player);
    }

    if (glm_vec3_dot(enemy_dir, enemy_to_player) <= 0.65f)
    {
      return false;
    }
  }

  // Then test if the enemy can see the player directly
  {
    vec3 ray_from, ray_to;
    glm_vec3_copy(transform[3], ray_from);
    ray_from[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS; // From feet to head
    cam_get_position(ray_to);

    vec3 ray;
    glm_vec3_sub(ray_to, ray_from, ray);

    const float old_dist = glm_vec3_norm(ray);

    vec3 n;
    collide_ray(ray_from, ray_to, ray_to, n);

    glm_vec3_sub(ray_to, ray_from, ray);

    const float new_dist = glm_vec3_norm(ray);

    if (new_dist < old_dist)
    {
      return false;
    }
  }

  // All checks passed
  return true;
}

void update_enemy(float delta_time)
{
  vec2 desired_velocity = GLM_VEC2_ZERO_INIT;
  float desired_angle_delta = 0.0f;
  bool should_respawn = false;

  // Determine if the player is visible from the perspective of the enemy
  const bool player_visible = calc_player_visible();

  switch (state)
  {
  case EnemyState_Walk:
    update_enemy_state_walk(transform[3], transform[2], player_visible, delta_time, desired_velocity,
                            &desired_angle_delta);
    break;
  case EnemyState_Aim:
    update_enemy_state_aim(transform[3], transform[2], player_visible, delta_time, desired_velocity,
                           &desired_angle_delta);
    break;
  case EnemyState_Fire:
    update_enemy_state_fire(transform, player_visible, delta_time, desired_velocity, &desired_angle_delta);
    break;
  case EnemyState_Flinch:
    update_enemy_state_flinch();
    break;
  case EnemyState_Die:
    update_enemy_state_die(transform[3], transform[2], delta_time, &desired_angle_delta, &should_respawn);
    break;
  }

  // Turn
  glm_rotate_y(transform, glm_rad(desired_angle_delta), transform);

  // Move
  vec3 old_pos, velocity;
  glm_vec3_copy(transform[3], old_pos);
  glm_translate_x(transform, desired_velocity[0]); // Untested as strafing is not implemented yet
  glm_translate_z(transform, desired_velocity[1]);
  glm_vec3_sub(transform[3], old_pos, velocity);

  // Simple collision
  {
    const float magnitude = glm_vec3_norm(velocity);
    const int step_count = (magnitude / ENEMY_COLLIDER_RADIUS) + 1;
    const float step_length = magnitude / step_count;

    vec3 step_vector;
    glm_vec3_scale_as(velocity, step_length, step_vector);

    old_pos[1] += ENEMY_COLLIDER_RADIUS; // From feet to capsule bottom center

    for (int step = 0; step < step_count; ++step)
    {
      glm_vec3_add(old_pos, step_vector, old_pos);

      vec3 top = { old_pos[0], old_pos[1] + ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS * 2, old_pos[2] };
      collide_capsule(top, old_pos, ENEMY_COLLIDER_RADIUS, CollisionPhase_Walls);
    }

    old_pos[1] -= ENEMY_COLLIDER_RADIUS; // From capsule bottom center to feet
    glm_vec3_copy(old_pos, transform[3]);
  }

  // Respawn
  if (should_respawn)
  {
    respawn_enemy(true);
  }

  // Always update animations and hitboxes
  aem_update_animation(render_info->model, mixer, delta_time, **joint_transforms);
  update_hitboxes();
}

void prepare_enemy_rendering()
{
  glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(render_info->model), joint_transforms,
               GL_DYNAMIC_DRAW);

  // Joint transform texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
}

struct ModelRenderInfo* get_enemy_render_info()
{
  return render_info;
}

void get_enemy_world_matrix(mat4 world_matrix)
{
  glm_mat4_copy(transform, world_matrix);
}

void debug_draw_enemy()
{
  {
    vec3 collider_bottom, collider_top;
    glm_vec3_copy(transform[3], collider_bottom);
    collider_bottom[1] += ENEMY_COLLIDER_RADIUS;

    glm_vec3_copy(collider_bottom, collider_top);
    collider_top[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS - ENEMY_COLLIDER_RADIUS;

    render_debug_manager_capsule(collider_bottom, collider_top, ENEMY_COLLIDER_RADIUS, GLM_ZUP);
  }

  render_debug_manager_capsule(hitbox_head_bottom, hitbox_head_top, ENEMY_HITBOX_HEAD_RADIUS, GLM_XUP);
  render_debug_manager_capsule(hitbox_upper_torso_bottom, hitbox_upper_torso_top, ENEMY_HITBOX_UPPER_TORSO_RADIUS,
                               GLM_XUP);
  render_debug_manager_capsule(hitbox_lower_torso_bottom, hitbox_lower_torso_top, ENEMY_HITBOX_LOWER_TORSO_RADIUS,
                               GLM_XUP);
}

static bool check_hitbox(vec3 from, vec3 to, vec3 hitbox_bottom, vec3 hitbox_top, float hitbox_radius)
{
  vec3 a, b;
  closest_segment_segment(from, to, hitbox_bottom, hitbox_top, a, b);
  glm_vec3_sub(a, b, a);

  const float dist = glm_vec3_norm(a);
  const bool hit = dist < hitbox_radius;

  if (hit)
  {
    glm_vec3_copy(b, to);
  }

  return hit;
}

enum EnemyHitArea is_enemy_hit(vec3 from, vec3 to)
{
  if (check_hitbox(from, to, hitbox_head_bottom, hitbox_head_top, ENEMY_HITBOX_HEAD_RADIUS))
  {
    return EnemyHitArea_Head;
  }

  if (check_hitbox(from, to, hitbox_upper_torso_bottom, hitbox_upper_torso_top, ENEMY_HITBOX_UPPER_TORSO_RADIUS))
  {
    return EnemyHitArea_UpperTorso;
  }

  if (check_hitbox(from, to, hitbox_lower_torso_bottom, hitbox_lower_torso_top, ENEMY_HITBOX_LOWER_TORSO_RADIUS))
  {
    return EnemyHitArea_LowerTorso;
  }

  return EnemyHitArea_None;
}

void enemy_hurt(float damage, vec3 dir)
{
  health -= damage;

  if (health <= 0.0f && preferences->ai_dying)
  {
    if (state != EnemyState_Die)
    {
      enter_enemy_state_die();
    }
  }
  else
  {
    enter_enemy_state_flinch();
  }
}

void free_enemy()
{
  aem_free_animation_mixer(mixer);

  glDeleteTextures(1, &joint_transform_texture);
  glDeleteBuffers(1, &joint_transform_buffer);

  free(joint_transforms);
}