#include "enemy.h"

#include "enemy_state_aim.h"
#include "enemy_state_chase.h"
#include "enemy_state_die.h"
#include "enemy_state_fire.h"
#include "enemy_state_flinch.h"
#include "enemy_state_roam.h"
#include "enemy_state_strafe.h"

#include "camera.h"
#include "collision.h"
#include "debug_manager.h"
#include "map.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <assert.h>

void respawn_enemy(struct Enemy* enemy, bool play_sound)
{
  // Reset health
  enemy->health = 100.0f;

  enemy->grounded = false;
  enemy->velocity_y = 0.0f;

  enemy->view_offset[0] = enemy->view_offset[1] = 0.0f;

  // Reset position and angle
  vec3 spawn_position;
  float spawn_yaw;
  get_current_map_random_enemy_spawn(spawn_position, &spawn_yaw);
  glm_translate_make(enemy->transform, spawn_position);
  glm_rotate_y(enemy->transform, glm_rad(spawn_yaw + 90.0f),
               enemy->transform); // TODO: This is a hack because the enemy doesn't yet use angles
  glm_scale(enemy->transform, (vec3){ ENEMY_SCALE, ENEMY_SCALE, ENEMY_SCALE });

  // Play the optional respawn sound effect
  if (play_sound)
  {
    play_respawn_sound(spawn_position);
  }

  // Start with the initial state: Roaming around
  enter_enemy_state_roam(enemy, true);
}

bool load_enemy(struct Enemy* enemy, const struct Preferences* preferences, const struct AEMModel* model)
{
  const uint32_t joint_count = aem_get_model_joint_count(model);

  if (aem_load_animation_mixer(joint_count, 4, 2, &enemy->mixer) != AEMAnimationMixerResult_Success)
  {
    return false;
  }

  aem_set_animation_mixer_enabled(enemy->mixer, true);
  aem_set_animation_mixer_blend_speed(enemy->mixer, 4.0f);

  load_enemy_state_roam(enemy, preferences, model);
  load_enemy_state_chase(enemy, preferences, model);
  load_enemy_state_aim(enemy, preferences);
  load_enemy_state_fire(enemy, preferences, model);
  load_enemy_state_strafe(enemy, preferences, model);
  load_enemy_state_flinch(enemy, model);
  load_enemy_state_die(enemy);

  // Enable skeletal animations
  {
    enemy->joint_transforms = malloc(joint_count * sizeof(*(enemy->joint_transforms)));
    assert(enemy->joint_transforms);

    for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
    {
      glm_mat4_identity(enemy->joint_transforms[joint_index]);
    }

    glGenBuffers(1, &enemy->joint_transform_buffer);
    glGenTextures(1, &enemy->joint_transform_texture);

    glBindBuffer(GL_TEXTURE_BUFFER, enemy->joint_transform_buffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * joint_count, NULL, GL_DYNAMIC_DRAW);

    glBindTexture(GL_TEXTURE_BUFFER, enemy->joint_transform_texture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, enemy->joint_transform_buffer);
  }

  respawn_enemy(enemy, false);

  return true;
}

static void update_enemy_hitboxes(struct Enemy* enemy, const struct AEMModel* model)
{
  // Head
  {
    mat4 hitbox_head_transform;
    aem_get_animation_mixer_joint_transform(model, enemy->mixer, ENEMY_HITBOX_HEAD_JOINT_INDEX, AEMAnimationLayer_Base,
                                            AEMJointTransformSpace_Global, (float*)hitbox_head_transform);
    glm_mat4_mul(enemy->transform, hitbox_head_transform, hitbox_head_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_HEAD_X, ENEMY_HITBOX_HEAD_BOTTOM_Y, ENEMY_HITBOX_HEAD_Z },
                  enemy->hitbox_head_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_HEAD_X, ENEMY_HITBOX_HEAD_TOP_Y, ENEMY_HITBOX_HEAD_Z }, enemy->hitbox_head_top);

    glm_mat4_mulv3(hitbox_head_transform, enemy->hitbox_head_bottom, 1.0f, enemy->hitbox_head_bottom);
    glm_mat4_mulv3(hitbox_head_transform, enemy->hitbox_head_top, 1.0f, enemy->hitbox_head_top);
  }

  // Upper torso
  {
    mat4 hitbox_upper_torso_transform;
    aem_get_animation_mixer_joint_transform(model, enemy->mixer, ENEMY_HITBOX_UPPER_TORSO_JOINT_INDEX,
                                            AEMAnimationLayer_Base, AEMJointTransformSpace_Global,
                                            (float*)hitbox_upper_torso_transform);
    glm_mat4_mul(enemy->transform, hitbox_upper_torso_transform, hitbox_upper_torso_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_UPPER_TORSO_X, ENEMY_HITBOX_UPPER_TORSO_BOTTOM_Y, ENEMY_HITBOX_UPPER_TORSO_Z },
                  enemy->hitbox_upper_torso_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_UPPER_TORSO_X, ENEMY_HITBOX_UPPER_TORSO_TOP_Y, ENEMY_HITBOX_UPPER_TORSO_Z },
                  enemy->hitbox_upper_torso_top);

    glm_mat4_mulv3(hitbox_upper_torso_transform, enemy->hitbox_upper_torso_bottom, 1.0f,
                   enemy->hitbox_upper_torso_bottom);
    glm_mat4_mulv3(hitbox_upper_torso_transform, enemy->hitbox_upper_torso_top, 1.0f, enemy->hitbox_upper_torso_top);
  }

  // Lower torso
  {
    mat4 hitbox_lower_torso_transform;
    aem_get_animation_mixer_joint_transform(model, enemy->mixer, ENEMY_HITBOX_LOWER_TORSO_JOINT_INDEX,
                                            AEMAnimationLayer_Base, AEMJointTransformSpace_Global,
                                            (float*)hitbox_lower_torso_transform);
    glm_mat4_mul(enemy->transform, hitbox_lower_torso_transform, hitbox_lower_torso_transform); // Model to world space

    glm_vec3_copy((vec3){ ENEMY_HITBOX_LOWER_TORSO_X, ENEMY_HITBOX_LOWER_TORSO_BOTTOM_Y, ENEMY_HITBOX_LOWER_TORSO_Z },
                  enemy->hitbox_lower_torso_bottom);
    glm_vec3_copy((vec3){ ENEMY_HITBOX_LOWER_TORSO_X, ENEMY_HITBOX_LOWER_TORSO_TOP_Y, ENEMY_HITBOX_LOWER_TORSO_Z },
                  enemy->hitbox_lower_torso_top);

    glm_mat4_mulv3(hitbox_lower_torso_transform, enemy->hitbox_lower_torso_bottom, 1.0f,
                   enemy->hitbox_lower_torso_bottom);
    glm_mat4_mulv3(hitbox_lower_torso_transform, enemy->hitbox_lower_torso_top, 1.0f, enemy->hitbox_lower_torso_top);
  }
}

static bool calc_player_visible(struct Enemy* enemy)
{
  // First test if the player is somewhat in front of the enemy
  {
    vec3 enemy_dir;
    glm_vec3_normalize_to(enemy->transform[2], enemy_dir);

    vec3 enemy_to_player;
    {
      vec3 player_pos;
      camera_get_position(player_pos);

      glm_vec3_sub(player_pos, enemy->transform[3], enemy_to_player);
      enemy_to_player[1] = 0.0f; // Flatten

      glm_vec3_normalize(enemy_to_player);
    }

    if (glm_vec3_dot(enemy_dir, enemy_to_player) <= 0.5f)
    {
      return false;
    }
  }

  // Then test if the enemy can see the player directly
  {
    vec3 ray_from, ray_to;
    glm_vec3_copy(enemy->transform[3], ray_from);
    ray_from[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS; // From feet to head
    camera_get_position(ray_to);

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

void update_enemy(const struct Preferences* preferences,
                  struct Enemy* enemy,
                  const struct AEMModel* model,
                  float delta_time)
{
  enemy->player_visible = calc_player_visible(enemy);

  // Set up the output state of the enemy
  static struct EnemyStateOutput state_output;
  glm_vec2_zero(state_output.movement);
  state_output.angle_delta = 0.0f;
  state_output.new_view_offset[0] = state_output.new_view_offset[1] = 0.0f;
  state_output.should_respawn = false;

  switch (enemy->state)
  {
  case EnemyState_Roam:
    update_enemy_state_roam(enemy, &state_output, delta_time);
    break;
  case EnemyState_Chase:
    update_enemy_state_chase(enemy, &state_output, delta_time);
    break;
  case EnemyState_Aim:
    update_enemy_state_aim(enemy, &state_output, delta_time);
    break;
  case EnemyState_Fire:
    update_enemy_state_fire(enemy, &state_output, delta_time);
    break;
  case EnemyState_Strafe:
    update_enemy_state_strafe(enemy, &state_output, delta_time);
    break;
  case EnemyState_Flinch:
    update_enemy_state_flinch(enemy);
    break;
  case EnemyState_Die:
    update_enemy_state_die(enemy, &state_output, delta_time);
    break;
  }

  // Turn the enemy transform
  glm_rotate_y(enemy->transform, glm_rad(state_output.angle_delta), enemy->transform);

  // View offset
  {
    enemy->view_offset[0] = state_output.new_view_offset[0]; // Pitch
    enemy->view_offset[1] = state_output.new_view_offset[1]; // Yaw

    mat4 view_offset = GLM_MAT4_IDENTITY_INIT;
    glm_rotate_y(view_offset, glm_rad(enemy->view_offset[1]), view_offset); // Yaw
    glm_rotate_x(view_offset, glm_rad(enemy->view_offset[0]), view_offset); // Pitch
    aem_set_animation_mixer_joint_transform(model, enemy->mixer, 12, AEMAnimationLayer_Additive,
                                            AEMJointTransformSpace_Global, (float*)view_offset);
  }

  // Calculate velocity
  vec3 velocity;
  {
    vec3 old_pos;
    glm_vec3_copy(enemy->transform[3], old_pos);
    glm_translate_x(enemy->transform, state_output.movement[0]);
    glm_translate_z(enemy->transform, state_output.movement[1]);
    glm_vec3_sub(enemy->transform[3], old_pos, velocity);
    velocity[1] = enemy->velocity_y;
  }

  // Simulate capsule
  {
    vec3 eye;
    glm_vec3_copy(enemy->transform[3], eye);
    eye[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS; // From feet to eye height

    simulate_capsule(preferences, eye, velocity, &enemy->grounded, ENEMY_COLLIDER_HEIGHT, ENEMY_COLLIDER_RADIUS,
                     delta_time);

    enemy->velocity_y = velocity[1];

    eye[1] -= ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS; // From eye to feet height
    glm_vec3_copy(eye, enemy->transform[3]);
  }

  // Respawn
  if (state_output.should_respawn)
  {
    respawn_enemy(enemy, true);
  }

  // Always update animations and hitboxes
  aem_update_animation(model, enemy->mixer, delta_time, **enemy->joint_transforms);
  update_enemy_hitboxes(enemy, model);

  // Update the joint transform buffer and texture
  glBindBuffer(GL_TEXTURE_BUFFER, enemy->joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(model), enemy->joint_transforms,
               GL_DYNAMIC_DRAW);
}

void bind_enemy_joint_transform_texture(const struct Enemy* enemy)
{
  // Bind the joint transform texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, enemy->joint_transform_texture);
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

enum EnemyHitArea is_enemy_hit(struct Enemy* enemy, vec3 from, vec3 to)
{
  if (check_hitbox(from, to, enemy->hitbox_head_bottom, enemy->hitbox_head_top, ENEMY_HITBOX_HEAD_RADIUS))
  {
    return EnemyHitArea_Head;
  }

  if (check_hitbox(from, to, enemy->hitbox_upper_torso_bottom, enemy->hitbox_upper_torso_top,
                   ENEMY_HITBOX_UPPER_TORSO_RADIUS))
  {
    return EnemyHitArea_UpperTorso;
  }

  if (check_hitbox(from, to, enemy->hitbox_lower_torso_bottom, enemy->hitbox_lower_torso_top,
                   ENEMY_HITBOX_LOWER_TORSO_RADIUS))
  {
    return EnemyHitArea_LowerTorso;
  }

  // If it wasn't a hit, check if it was close
  if (enemy->state == EnemyState_Roam)
  {
    vec3 collider_bottom, collider_top;
    glm_vec3_copy(enemy->transform[3], collider_bottom);
    collider_bottom[1] += ENEMY_COLLIDER_RADIUS;

    vec3 a, b;
    closest_segment_segment(from, to, collider_bottom, collider_top, a, b);
    glm_vec3_sub(a, b, a);

    const float dist = glm_vec3_norm(a);
    const bool in_hearing_distance = dist < 5.0f;

    // The enemy heard the shot and starts chasing, aiming and returning fire
    if (in_hearing_distance)
    {
      enter_enemy_state_chase(enemy);
    }
  }

  return EnemyHitArea_None;
}

void hurt_enemy(struct Enemy* enemy, struct Preferences* preferences, float damage, vec3 dir)
{
  enemy->health -= damage;

  if (enemy->health <= 0.0f && preferences->ai_dying)
  {
    if (enemy->state != EnemyState_Die)
    {
      enter_enemy_state_die(enemy);
    }
  }
  else
  {
    enter_enemy_state_flinch(enemy);
  }
}

void debug_draw_enemy(struct Enemy* enemy)
{
  {
    vec3 collider_bottom, collider_top;
    glm_vec3_copy(enemy->transform[3], collider_bottom);
    collider_bottom[1] += ENEMY_COLLIDER_RADIUS;

    glm_vec3_copy(collider_bottom, collider_top);
    collider_top[1] += ENEMY_COLLIDER_HEIGHT - ENEMY_COLLIDER_RADIUS - ENEMY_COLLIDER_RADIUS;

    render_debug_manager_capsule(collider_bottom, collider_top, ENEMY_COLLIDER_RADIUS, GLM_ZUP);
  }

  render_debug_manager_capsule(enemy->hitbox_head_bottom, enemy->hitbox_head_top, ENEMY_HITBOX_HEAD_RADIUS, GLM_XUP);
  render_debug_manager_capsule(enemy->hitbox_upper_torso_bottom, enemy->hitbox_upper_torso_top,
                               ENEMY_HITBOX_UPPER_TORSO_RADIUS, GLM_XUP);
  render_debug_manager_capsule(enemy->hitbox_lower_torso_bottom, enemy->hitbox_lower_torso_top,
                               ENEMY_HITBOX_LOWER_TORSO_RADIUS, GLM_XUP);
}

void free_enemy(struct Enemy* enemy)
{
  aem_free_animation_mixer(enemy->mixer);

  glDeleteTextures(1, &enemy->joint_transform_texture);
  glDeleteBuffers(1, &enemy->joint_transform_buffer);

  free(enemy->joint_transforms);
}