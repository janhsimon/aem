#include "view_model.h"

#include "camera.h"
#include "renderer.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <glad/gl.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static const struct AEMModel* model = NULL;

static GLuint joint_transform_buffer, joint_transform_texture;
static mat4* joint_transforms;

static struct AEMAnimationMixer* mixer;
static struct AEMAnimationChannel *reload_channel, *shoot_channel;

static bool prev_moving = false;
static bool is_reloading = false, is_shooting = false;

bool load_view_model(const struct AEMModel* model_)
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

  // Idle
  {
    struct AEMAnimationChannel* channel = aem_get_animation_mixer_channel(mixer, 0);
    channel->animation_index = 1;
    channel->is_playing = true;
  }

  // Walking
  {
    struct AEMAnimationChannel* channel = aem_get_animation_mixer_channel(mixer, 1);
    channel->animation_index = 9;
    channel->is_playing = true;
    channel->playback_speed = 4.0f;
  }

  // Reloading
  {
    reload_channel = aem_get_animation_mixer_channel(mixer, 2);
    reload_channel->animation_index = 12;
    reload_channel->is_looping = false;
  }

  // Shooting
  {
    shoot_channel = aem_get_animation_mixer_channel(mixer, 3);
    shoot_channel->animation_index = 2;
    shoot_channel->is_looping = false;
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

void update_view_model(bool moving, float delta_time)
{
  if (is_shooting)
  {
    if (shoot_channel->time >= aem_get_model_animation_duration(model, 2) - 0.1f)
    {
      is_shooting = false;
      aem_set_animation_mixer_blend_speed(mixer, 10.0f);
      aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk
    }
  }
  else
  {
    if (get_shoot_button_down() && !is_reloading)
    {
      is_shooting = true;
      shoot_channel->time = 0.0f;
      shoot_channel->is_playing = true;
      aem_set_animation_mixer_blend_speed(mixer, 10.0f);
      aem_blend_to_animation_mixer_channel(mixer, 3); // To shoot
    }
    else
    {
      if (is_reloading)
      {
        if (reload_channel->time >= aem_get_model_animation_duration(model, 12) - 0.2f)
        {
          is_reloading = false;
          aem_set_animation_mixer_blend_speed(mixer, 10.0f);
          aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk
        }
      }
      else
      {
        if (get_reload_key_down())
        {
          is_reloading = true;
          reload_channel->time = 0.0f;
          reload_channel->is_playing = true;
          aem_set_animation_mixer_blend_speed(mixer, 10.0f);
          aem_blend_to_animation_mixer_channel(mixer, 2); // To reload
        }
        else
        {
          if (moving != prev_moving)
          {
            aem_set_animation_mixer_blend_speed(mixer, 5.0f);
            aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk
          }
        }
      }
    }
  }

  aem_update_animation(model, mixer, delta_time, **joint_transforms);

  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(model), joint_transforms, GL_DYNAMIC_DRAW);

  prev_moving = moving;
}

void prepare_view_model_rendering(float aspect)
{
  // Reduced fixed fov
  use_fov(aspect, 50.0f);

  // World matrix
  {
    mat4 world_matrix;

    // Copy the camera position
    vec3 camera_position;
    cam_get_position(camera_position);
    glm_translate_make((vec4*)world_matrix, camera_position);

    // And insert the camera orientation
    mat3 camera_orientation;
    cam_get_orientation((float*)camera_orientation);
    glm_mat4_ins3(camera_orientation, (vec4*)world_matrix);

    // Offset slightly
    glm_translate_y((vec4*)world_matrix, -0.02f);
    glm_translate_z((vec4*)world_matrix, 0.1f);

    // Scale way down
    glm_scale_uni((vec4*)world_matrix, 0.02f);
    use_world_matrix((float*)world_matrix);
  }

  // Joint transform texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
}

void free_view_model()
{
  aem_free_animation_mixer(mixer);

  glDeleteTextures(1, &joint_transform_texture);
  glDeleteBuffers(1, &joint_transform_buffer);

  free(joint_transforms);
}