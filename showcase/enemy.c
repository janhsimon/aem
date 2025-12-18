#include "enemy.h"

#include "collision.h"
#include "debug_renderer.h"
#include "map.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define ENEMY_COLLIDER_RADIUS 0.5f
#define ENEMY_COLLIDER_HEIGHT 1.8f

#define ENEMY_MOVE_SPEED 0.1f
#define ENEMY_TURN_ANGLE 2.5f

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

#define ENEMY_WALK_ANIMATION_INDEX 1
#define ENEMY_DIE_ANIMATION_INDEX 15

static const struct AEMModel* model = NULL;

static GLuint joint_transform_buffer, joint_transform_texture;
static mat4* joint_transforms;

static struct AEMAnimationMixer* mixer;
static struct AEMAnimationChannel *walk_channel, *flinch_channel;

static mat4 transform;
static float rot = 0.0f;

static vec3 hitbox_head_bottom, hitbox_head_top;
static vec3 hitbox_upper_torso_bottom, hitbox_upper_torso_top;
static vec3 hitbox_lower_torso_bottom, hitbox_lower_torso_top;

static float health;

static void respawn_enemy(bool play_sound)
{
  // Reset health
  health = 100.0f;

  // Reset animation to walking
  walk_channel->time = 0.0f;
  walk_channel->animation_index = ENEMY_WALK_ANIMATION_INDEX;
  walk_channel->playback_speed = 1.75f;
  walk_channel->is_looping = true;

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
}

bool load_enemy(const struct AEMModel* model_)
{
  model = model_;

  glGenBuffers(1, &joint_transform_buffer);
  glGenTextures(1, &joint_transform_texture);

  const uint32_t joint_count = aem_get_model_joint_count(model);

  if (aem_load_animation_mixer(joint_count, 4, &mixer) != AEMAnimationMixerResult_Success)
  {
    return false;
  }

  aem_set_animation_mixer_enabled(mixer, true);

  // Walking
  {
    walk_channel = aem_get_animation_mixer_channel(mixer, 0);
    walk_channel->animation_index = ENEMY_WALK_ANIMATION_INDEX;
    walk_channel->playback_speed = 1.75f;
    walk_channel->is_playing = true;
  }

  // Flinching
  {
    flinch_channel = aem_get_animation_mixer_channel(mixer, 1);
    flinch_channel->animation_index = ENEMY_DIE_ANIMATION_INDEX;
    flinch_channel->playback_speed = 1.5f;
    flinch_channel->is_playing = true;
  }

  // Enable skeletal animations
  {
    joint_transforms = malloc(joint_count * sizeof(mat4));
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

void update_enemy(const struct Preferences* preferences, float delta_time)
{
  // Update animation
  aem_update_animation(model, mixer, delta_time, **joint_transforms);

  if (health <= 0.0f)
  {
    // Handle respawning when the death animation has run out
    if (walk_channel->animation_index == ENEMY_DIE_ANIMATION_INDEX)
    {
      const float duration = aem_get_model_animation_duration(model, ENEMY_DIE_ANIMATION_INDEX);
      if (walk_channel->time >= duration)
      {
        respawn_enemy(true);
      }
    }
  }
  else
  {
    // Handle the hurt animation
    if (flinch_channel->weight > 0.0f)
    {
      const float duration = aem_get_model_animation_duration(model, ENEMY_DIE_ANIMATION_INDEX);

      flinch_channel->weight = 1.0f - flinch_channel->time * (3.0f / duration);

      if (flinch_channel->weight <= 0.0f)
      {
        flinch_channel->weight = 0.0f;
      }
    }
    else
    {
      // Movement
      vec3 old_pos, velocity;
      {
        glm_vec3_copy(transform[3], old_pos);

        if (preferences->ai_turning)
        {
          glm_rotate_y(transform, glm_rad(rot), transform);
        }

        if (preferences->ai_walking)
        {
          glm_translate_z(transform, ENEMY_MOVE_SPEED * delta_time);
        }

        glm_vec3_sub(transform[3], old_pos, velocity);
      }

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

      // Random turns
      if (rand() % 100 == 22)
      {
        rot = ((rand() % 100) / 100.0f) * ENEMY_TURN_ANGLE - (ENEMY_TURN_ANGLE / 2.0f);
      }

      // Footstep sounds
      {
        static int footstep_counter = 0;

        static float duration = 0.0f;
        if (duration <= 0.0f)
        {
          duration = aem_get_model_animation_duration(model, 1);
        }

        float relative_time = walk_channel->time / duration;
        if (relative_time < 0.0f)
        {
          relative_time += 1.0f;
        }

        for (int step_index = 0; step_index < 2; ++step_index)
        {
          const float period_start = 0.5f * step_index + 0.25f;
          if (footstep_counter == step_index && relative_time >= period_start && relative_time < period_start + 0.5f)
          {
            const int sound_index = (rand() % 2) * 2 + (step_index % 2);

            vec3 feet;
            glm_vec3_copy(transform[3], feet);
            play_enemy_footstep_sound(sound_index, feet);

            footstep_counter = (footstep_counter + 1) % 2;
            break;
          }
        }
      }
    }
  }

  // Update hitboxes
  {
    // Head
    {
      mat4 hitbox_head_transform;
      aem_get_animation_mixer_joint_transform(model, mixer, ENEMY_HITBOX_HEAD_JOINT_INDEX,
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
      aem_get_animation_mixer_joint_transform(model, mixer, ENEMY_HITBOX_UPPER_TORSO_JOINT_INDEX,
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
      aem_get_animation_mixer_joint_transform(model, mixer, ENEMY_HITBOX_LOWER_TORSO_JOINT_INDEX,
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
}

void prepare_enemy_rendering()
{
  glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(model), joint_transforms, GL_DYNAMIC_DRAW);

  // Joint transform texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
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

    debug_render_capsule(collider_bottom, collider_top, ENEMY_COLLIDER_RADIUS, GLM_ZUP);
  }

  debug_render_capsule(hitbox_head_bottom, hitbox_head_top, ENEMY_HITBOX_HEAD_RADIUS, GLM_XUP);
  debug_render_capsule(hitbox_upper_torso_bottom, hitbox_upper_torso_top, ENEMY_HITBOX_UPPER_TORSO_RADIUS, GLM_XUP);
  debug_render_capsule(hitbox_lower_torso_bottom, hitbox_lower_torso_top, ENEMY_HITBOX_LOWER_TORSO_RADIUS, GLM_XUP);
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

void enemy_hurt(const struct Preferences* preferences, float damage, vec3 dir)
{
  health -= damage;

  if (health <= 0.0f)
  {
    if (walk_channel->animation_index == ENEMY_DIE_ANIMATION_INDEX || !preferences->ai_death)
    {
      return;
    }

    // Play death animation
    walk_channel->animation_index = ENEMY_DIE_ANIMATION_INDEX;
    walk_channel->playback_speed = 1.0f;
    walk_channel->is_looping = false;
    walk_channel->time = 0.0f;

    // Make sure the flinch animation doesn't play anymore
    flinch_channel->weight = 0.0f;

    dir[1] = 0.0f; // Flatten
    glm_normalize(dir);

    // Face the direction the bullet came from
    {
      vec3 x;
      glm_vec3_cross(dir, GLM_YUP, x);
      glm_vec3_copy(x, transform[0]);
      glm_vec3_copy(GLM_YUP, transform[1]);
      glm_vec3_copy(dir, transform[2]);

      glm_scale(transform, (vec3){ 19.25f, 19.25f, 19.25f });
    }
  }
  else
  {
    // Play flinch animation
    flinch_channel->time = 0.0f;
    flinch_channel->weight = 1.0f;
  }
}

void free_enemy()
{
  aem_free_animation_mixer(mixer);

  glDeleteTextures(1, &joint_transform_texture);
  glDeleteBuffers(1, &joint_transform_buffer);

  free(joint_transforms);
}