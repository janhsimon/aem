#include "view_model.h"

#include "camera.h"
#include "collision.h"
#include "debug_renderer.h"
#include "enemy.h"
#include "input.h"
#include "preferences.h"
#include "sound.h"

#include <aem/animation_mixer.h>
#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <glad/gl.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define IDLE_ANIMATION_INDEX 1
#define WALK_ANIMATION_INDEX 2   // AK: 9
#define SHOOT_ANIMATION_INDEX 4  // AK: 2
#define RELOAD_ANIMATION_INDEX 6 // AK: 12

#define SHOT_COOLDOWN 0.1f

#define BULLET_COUNT 30

static const struct AEMModel* model = NULL;

static GLuint joint_transform_buffer, joint_transform_texture;
static mat4* joint_transforms;

static struct AEMAnimationMixer* mixer;
static struct AEMAnimationChannel *walk_channel, *reload_channel, *shoot_channel;

static bool prev_moving = false;
static bool is_reloading = false, is_shooting = false;

static float shot_cooldown = 0.0f;
static float moving_start_time; // Used for footstep sounds

static int ammo = BULLET_COUNT;

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
    walk_channel = aem_get_animation_mixer_channel(mixer, IDLE_ANIMATION_INDEX);
    walk_channel->animation_index = WALK_ANIMATION_INDEX;
    walk_channel->is_playing = true;
    // walk_channel->playback_speed = 4.0f;
  }

  // Reloading
  {
    reload_channel = aem_get_animation_mixer_channel(mixer, 2);
    reload_channel->animation_index = RELOAD_ANIMATION_INDEX;
    reload_channel->is_looping = false;
  }

  // Shooting
  {
    shoot_channel = aem_get_animation_mixer_channel(mixer, 3);
    shoot_channel->animation_index = SHOOT_ANIMATION_INDEX;
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
  shot_cooldown -= delta_time;
  show_muzzleflash = false;

  if (is_shooting)
  {
    const float duration = aem_get_model_animation_duration(model, SHOOT_ANIMATION_INDEX);
    if (shoot_channel->time < duration / 4.0f)
    {
      show_muzzleflash = true;
    }

    if (shoot_channel->time >= duration - 0.165f)
    {
      is_shooting = false;
      aem_set_animation_mixer_blend_speed(mixer, 10.0f);
      aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk
    }
  }
  // else
  {
    if (get_shoot_button_down() && !is_reloading && shot_cooldown <= 0.0f)
    {
      if (ammo <= 0)
      {
        play_ak47_dry_sound();
        shot_cooldown = SHOT_COOLDOWN * 4.0f;
      }
      else
      {

        is_shooting = true;
        shoot_channel->time = 0.0f;
        shoot_channel->is_playing = true;
        aem_set_animation_mixer_blend_speed(mixer, 10.0f);
        aem_blend_to_animation_mixer_channel(mixer, 3); // To shoot

        play_ak47_fire_sound();

        vec3 from;
        cam_get_position(from);

        mat3 cam_dir;
        cam_get_orientation(cam_dir);

        vec3 ray = { 0.0f, 0.0f, 1.0f };
        glm_mat3_mulv(cam_dir, ray, ray);

        glm_vec3_scale_as(ray, 10000.0f, ray);

        vec3 to;
        glm_vec3_add(from, ray, to);

        collide_ray(from, to, to);
        add_debug_line(from, to);

        if (is_enemy_hit(from, to))
        {
          glm_vec3_negate(ray);
          enemy_die(ray);

          play_headshot_sound();
        }
        else
        {
          play_impact_sound(to);
        }

        --ammo;
        shot_cooldown = SHOT_COOLDOWN;

        // Randomize muzzleflash
        {
          muzzleflash_angle = glm_rad(rand() % 4 * 90.0f + (rand() % 40) - 20.0f);
          muzzleflash_scale = (rand() % 10) * 0.04f + 0.4f;
        }
      }
    }
    else
    {
      if (is_reloading)
      {
        const float duration = aem_get_model_animation_duration(model, RELOAD_ANIMATION_INDEX);
        if (reload_channel->time >= aem_get_model_animation_duration(model, RELOAD_ANIMATION_INDEX) - 0.2f)
        {
          is_reloading = false;
          aem_set_animation_mixer_blend_speed(mixer, 10.0f);
          aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk
        }
        else if (reload_channel->time >= duration / 2.0f && ammo != BULLET_COUNT)
        {
          ammo = BULLET_COUNT;
        }
      }
      else
      {
        if (get_reload_key_down() && ammo < BULLET_COUNT)
        {
          is_reloading = true;
          reload_channel->time = 0.0f;
          reload_channel->is_playing = true;
          aem_set_animation_mixer_blend_speed(mixer, 10.0f);
          aem_blend_to_animation_mixer_channel(mixer, 2); // To reload

          play_ak47_reload_sound();
        }
        else
        {
          if (moving != prev_moving)
          {
            aem_set_animation_mixer_blend_speed(mixer, 5.0f);
            aem_blend_to_animation_mixer_channel(mixer, (uint32_t)moving); // To idle or walk

            if (moving)
            {
              moving_start_time = walk_channel->time;
            }
          }
        }
      }
    }
  }

  aem_update_animation(model, mixer, delta_time, **joint_transforms);

  // Footstep sounds
  if (moving)
  {
    static int footstep_counter = 0;

    static float duration = 0.0f;
    if (duration <= 0.0f)
    {
      duration = aem_get_model_animation_duration(model, WALK_ANIMATION_INDEX);
    }

    float relative_time = (walk_channel->time - moving_start_time) / duration;
    if (relative_time < 0.0f)
    {
      relative_time += 1.0f;
    }

    for (int step_index = 0; step_index < 8; ++step_index)
    {
      const float period_start = 0.125f * step_index + 0.05f;
      if (footstep_counter == step_index && relative_time >= period_start && relative_time < period_start + 0.125f)
      {
        const int sound_index = (rand() % 2) * 2 + (step_index % 2);
        play_player_footstep_sound(sound_index);

        footstep_counter = (footstep_counter + 1) % 8;
        break;
      }
    }
  }

  prev_moving = moving;
}

void view_model_get_world_matrix(struct Preferences* preferences, mat4 world_matrix)
{
  // Copy the camera position
  vec3 camera_position;
  cam_get_position(camera_position);
  glm_translate_make((vec4*)world_matrix, camera_position);

  // And insert the camera orientation
  mat3 camera_orientation;
  cam_get_orientation(camera_orientation);
  glm_mat4_ins3(camera_orientation, world_matrix);

  glm_translate(world_matrix, preferences->view_model_position);
  glm_scale_uni(world_matrix, preferences->view_model_scale);
}

void prepare_view_model_rendering(float aspect)
{
  glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * aem_get_model_joint_count(model), joint_transforms, GL_DYNAMIC_DRAW);

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

int view_model_get_ammo()
{
  return ammo;
}