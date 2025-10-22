#include "enemy.h"

#include "collision.h"
#include "renderer.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/mat4.h>

#include <glad/gl.h>

#include <assert.h>
#include <stdlib.h>

#define ENEMY_RADIUS 0.5f
#define ENEMY_HEIGHT 1.8f

static const struct AEMModel* model = NULL;

static GLuint joint_transform_buffer, joint_transform_texture;
static mat4* joint_transforms;

static struct AEMAnimationMixer* mixer;
static struct AEMAnimationChannel* channel;

static mat4 transform;
static float rot = 0.2f;

// enum
//{
//   EnemyState_WalkingLeft,
//   EnemyState_WalkingRight
// } state = EnemyState_WalkingLeft;

bool alive = true;

bool load_enemy(const struct AEMModel* model_)
{
  model = model_;

  glm_scale_make(transform, (vec3){ 19.25f, 19.25f, 19.25f });
  glm_translate(transform, (vec3){ -0.1f, 0.0f, -0.22f });

  glGenBuffers(1, &joint_transform_buffer);
  glGenTextures(1, &joint_transform_texture);

  const uint32_t joint_count = aem_get_model_joint_count(model);

  if (aem_load_animation_mixer(joint_count, 4, &mixer) != AEMAnimationMixerResult_Success)
  {
    return false;
  }

  aem_set_animation_mixer_enabled(mixer, true);

  // Idle
  {
    channel = aem_get_animation_mixer_channel(mixer, 0);
    channel->animation_index = 1;
    channel->playback_speed = 1.75f;
    channel->is_playing = true;
  }

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

  return true;
}

void update_enemy(float delta_time)
{
  if (!alive)
  {
    aem_update_animation(model, mixer, delta_time, **joint_transforms);

    if (channel->animation_index == 15)
    {
      const float duration = aem_get_model_animation_duration(model, 15);
      if (channel->time >= duration)
      {
        channel->animation_index = 16;
        channel->time = 0.0f;
      }
    }
    else if (channel->animation_index == 16)
    {
      const float duration = aem_get_model_animation_duration(model, 16);
      if (channel->time >= duration)
      {
        channel->time = 0.0f;
        channel->animation_index = 1;
        channel->playback_speed = 1.75f;
        channel->is_looping = true;
        alive = true;
      }
    }

    return;
  }

  vec3 old_pos;
  glm_vec3_copy(transform[3], old_pos);

  // Update position
  {
    /*if (state == EnemyState_WalkingLeft)
    {
      glm_rotate_y(transform, glm_rad(0.2f), transform);
    }
    else if (state == EnemyState_WalkingRight)
    {
      glm_rotate_y(transform, glm_rad(-0.2f), transform);
    }*/

    glm_rotate_y(transform, glm_rad(rot), transform);

    glm_translate_z(transform, 0.1f * delta_time);
  }

  vec3 velocity;
  glm_vec3_sub(transform[3], old_pos, velocity);

  // New-style collision
  {
    const float magnitude = glm_vec3_norm(velocity);
    const int step_count = (magnitude / ENEMY_RADIUS) + 1;
    const float step_length = magnitude / step_count;

    vec3 step_vector;
    glm_vec3_scale_as(velocity, step_length, step_vector);

    old_pos[1] += ENEMY_RADIUS; // From feet to capsule bottom center

    for (int step = 0; step < step_count; ++step)
    {
      glm_vec3_add(old_pos, step_vector, old_pos);

      vec3 top;
      top[0] = old_pos[0];
      top[1] = old_pos[1] + ENEMY_HEIGHT - ENEMY_RADIUS - ENEMY_RADIUS;
      top[2] = old_pos[2];

      collide_capsule(top, old_pos, ENEMY_RADIUS);
    }

    old_pos[1] -= ENEMY_RADIUS; // From capsule bottom center to feet
    glm_vec3_copy(old_pos, transform[3]);
  }

  // Update animation
  aem_update_animation(model, mixer, delta_time, **joint_transforms);

  if (rand() % 100 == 22)
  {
    rot = ((rand() % 100) / 100.0f) * 2.5f - 1.25f;
  }
}

void prepare_enemy_rendering()
{
  use_world_matrix((float*)transform);

  glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(model), joint_transforms, GL_DYNAMIC_DRAW);

  // Joint transform texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
}

bool is_enemy_hit(vec3 from, vec3 to)
{
  vec3 base;
  glm_vec3_copy(transform[3], base);
  base[1] += ENEMY_RADIUS; // From feet to capsule bottom center

  vec3 top;
  top[0] = base[0];
  top[1] = base[1] + ENEMY_HEIGHT - ENEMY_RADIUS - ENEMY_RADIUS;
  top[2] = base[2];

  vec3 out_a, out_b;
  closest_segment_segment(from, to, base, top, out_a, out_b);

  vec3 i;
  glm_vec3_sub(out_a, out_b, i);

  return glm_vec3_norm(i) < ENEMY_RADIUS;
}

void enemy_die(vec3 dir)
{
  if (!alive)
  {
    return;
  }

  channel->animation_index = 15;
  channel->playback_speed = 1.0f;
  channel->is_looping = false;
  channel->time = 0.0f;

  glm_normalize(dir);

  vec3 x;
  glm_vec3_cross(dir, GLM_YUP, x);
  glm_vec3_copy(x, transform[0]);
  glm_vec3_copy(GLM_YUP, transform[1]);
  glm_vec3_copy(dir, transform[2]);

  glm_scale(transform, (vec3){ 19.25f, 19.25f, 19.25f });

  alive = false;
}

void free_enemy()
{
  aem_free_animation_mixer(mixer);

  glDeleteTextures(1, &joint_transform_texture);
  glDeleteBuffers(1, &joint_transform_buffer);

  free(joint_transforms);
}