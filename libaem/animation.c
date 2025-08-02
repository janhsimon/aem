#include "animation_mixer.h"
#include "common.h"
#include "model.h"

#include <cglm/mat4.h>
#include <cglm/quat.h>

static uint32_t get_keyframe_index_after(float time, struct Keyframe* keyframes, uint32_t keyframe_count)
{
  uint32_t after_index = keyframe_count;
  for (uint32_t keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
  {
    if (keyframes[keyframe_index].time >= time)
    {
      after_index = keyframe_index;
      break;
    }
  }

  return after_index;
}

static void get_keyframe_blend_vec3(float time, struct Keyframe* keyframes, uint32_t keyframe_count, vec3 out)
{
  uint32_t keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

  // Before the first keyframe
  if (keyframe_index == 0)
  {
    glm_vec3_make(keyframes[0].data, out);
  }
  // After the last keyframe
  else if (keyframe_index == keyframe_count)
  {
    glm_vec3_make(keyframes[keyframe_count - 1].data, out);
  }
  // Blend keyframes in the middle
  else
  {
    struct Keyframe* keyframe_from = &keyframes[keyframe_index - 1];
    struct Keyframe* keyframe_to = &keyframes[keyframe_index];

    vec3 from;
    glm_vec3_make(keyframe_from->data, from);

    vec3 to;
    glm_vec3_make(keyframe_to->data, to);

    const float blend = (time - keyframe_from->time) / (keyframe_to->time - keyframe_from->time);
    glm_vec3_lerp(from, to, blend, out);
  }
}

static void get_keyframe_blend_quat(float time, struct Keyframe* keyframes, uint32_t keyframe_count, versor out)
{
  uint32_t keyframe_index = get_keyframe_index_after(time, keyframes, keyframe_count);

  // Before the first keyframe
  if (keyframe_index == 0)
  {
    glm_quat_make(keyframes[0].data, out);
  }
  // After the last keyframe
  else if (keyframe_index == keyframe_count)
  {
    glm_quat_make(keyframes[keyframe_count - 1].data, out);
  }
  // Blend keyframes in the middle
  else
  {
    struct Keyframe* keyframe_from = &keyframes[keyframe_index - 1];
    struct Keyframe* keyframe_to = &keyframes[keyframe_index];

    versor from;
    glm_quat_make(keyframe_from->data, from);

    versor to;
    glm_quat_make(keyframe_to->data, to);

    float blend = (time - keyframe_from->time) / (keyframe_to->time - keyframe_from->time);
    glm_quat_slerp(from, to, blend, out);
  }
}

static void get_joint_posed_transform_local_trs(const struct AEMModel* model,
                                                uint32_t joint_index,
                                                int32_t animation_index,
                                                float time,
                                                vec3 translation,
                                                versor rotation,
                                                vec3 scale)
{
  const struct Track* track = &model->tracks[animation_index * model->header.joint_count + joint_index];

  const uint32_t translation_keyframe_count = track->translation_keyframe_count;
  if (translation_keyframe_count > 0)
  {
    struct Keyframe* translation_keyframes = &model->keyframes[track->first_keyframe_index];

    get_keyframe_blend_vec3(time, translation_keyframes, translation_keyframe_count, translation);
  }
  else
  {
    glm_vec3_zero(translation);
  }

  const uint32_t rotation_keyframe_count = track->rotation_keyframe_count;
  if (rotation_keyframe_count > 0)
  {
    struct Keyframe* rotation_keyframes =
      &model->keyframes[track->first_keyframe_index + track->translation_keyframe_count];

    get_keyframe_blend_quat(time, rotation_keyframes, rotation_keyframe_count, rotation);
  }
  else
  {
    glm_quat_identity(rotation);
  }

  const uint32_t scale_keyframe_count = track->scale_keyframe_count;
  if (scale_keyframe_count > 0)
  {
    struct Keyframe* scale_keyframes =
      &model
         ->keyframes[track->first_keyframe_index + track->translation_keyframe_count + track->rotation_keyframe_count];

    get_keyframe_blend_vec3(time, scale_keyframes, scale_keyframe_count, scale);
  }
  else
  {
    glm_vec3_one(scale);
  }
}

enum AEMAnimationMixerResult
aem_load_animation_mixer(uint32_t joint_count, uint32_t channel_count, struct AEMAnimationMixer** mixer)
{
  *mixer = malloc(sizeof(struct AEMAnimationMixer));
  if (!*mixer)
  {
    return AEMAnimationMixerResult_OutOfMemory;
  }

  (*mixer)->channels = malloc(sizeof(*(*mixer)->channels) * channel_count);
  if (!(*mixer)->channels)
  {
    return AEMAnimationMixerResult_OutOfMemory;
  }

  for (uint32_t channel_index = 0; channel_index < channel_count; ++channel_index)
  {
    struct AEMAnimationChannel* channel = &(*mixer)->channels[channel_index];

    channel->animation_index = 0;
    channel->is_playing = false;
    channel->is_looping = true;
    channel->playback_speed = 1.0f;
    channel->time = 0.0f;

    channel->weight = (channel_index == 0) ? 1.0f : 0.0f;
  }

  (*mixer)->joint_transforms = malloc(sizeof(mat4) * joint_count);
  if (!(*mixer)->joint_transforms)
  {
    return AEMAnimationMixerResult_OutOfMemory;
  }

  (*mixer)->channel_count = channel_count;
  (*mixer)->joint_count = joint_count;

  (*mixer)->is_enabled = false;

  (*mixer)->is_blending = false;
  (*mixer)->blend_target_channel_index = 0;
  (*mixer)->blend_target_channel_initial_weight = 0.0f;
  (*mixer)->blend_progress = 0.0f;
  (*mixer)->blend_speed = 1.0f;
  (*mixer)->blend_mode = AEMAnimationBlendMode_Smooth;

  return AEMAnimationMixerResult_Success;
}

bool aem_get_animation_mixer_enabled(const struct AEMAnimationMixer* mixer)
{
  return mixer->is_enabled;
}

void aem_set_animation_mixer_enabled(struct AEMAnimationMixer* mixer, bool enabled)
{
  mixer->is_enabled = enabled;
}

float aem_get_animation_mixer_blend_speed(const struct AEMAnimationMixer* mixer)
{
  return mixer->blend_speed;
}

void aem_set_animation_mixer_blend_speed(struct AEMAnimationMixer* mixer, float blend_speed)
{
  mixer->blend_speed = blend_speed;
}

enum AEMAnimationBlendMode aem_get_animation_mixer_blend_mode(const struct AEMAnimationMixer* mixer)
{
  return mixer->blend_mode;
}

void aem_set_animation_mixer_blend_mode(struct AEMAnimationMixer* mixer, enum AEMAnimationBlendMode blend_mode)
{
  mixer->blend_mode = blend_mode;
}

void aem_free_animation_mixer(struct AEMAnimationMixer* mixer)
{
  free(mixer->channels);
  free(mixer->joint_transforms);
  free(mixer);
}

struct AEMAnimationChannel*
aem_get_animation_mixer_channel(const struct AEMAnimationMixer* mixer, uint32_t channel_index)
{
  return &mixer->channels[channel_index];
}

void aem_blend_to_animation_mixer_channel(struct AEMAnimationMixer* mixer, uint32_t channel_index)
{
  mixer->is_blending = true;
  mixer->blend_target_channel_index = channel_index;
  mixer->blend_target_channel_initial_weight = mixer->channels[channel_index].weight;
  mixer->blend_progress = 0.0f;
}

static float smoothstep(float x)
{
  return x * x * (3.0f - 2.0f * x);
}

static float smootherstep(float x)
{
  return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

void aem_update_animation(const struct AEMModel* model,
                          struct AEMAnimationMixer* mixer,
                          float delta_time,
                          float* joint_transforms)
{
  mat4* transforms = (mat4*)joint_transforms;

  // Show the bind pose and early out if the mixer is not enabled
  if (!mixer->is_enabled)
  {
    for (uint32_t joint_index = 0; joint_index < mixer->joint_count; ++joint_index)
    {
      glm_mat4_identity(transforms[joint_index]);
    }

    return;
  }

  // Play channels
  for (uint32_t channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
  {
    struct AEMAnimationChannel* channel = &mixer->channels[channel_index];

    if (channel->is_playing)
    {
      channel->time += channel->playback_speed * delta_time;

      const float duration = model->animations[channel->animation_index].duration;
      if (channel->is_looping)
      {
        while (channel->time > duration)
        {
          channel->time -= duration;
        }
      }
      else
      {
        if (channel->time > duration)
        {
          channel->time = duration;
        }
      }
    }
  }

  // Blend automatically
  if (mixer->is_blending)
  {
    struct AEMAnimationChannel* target_channel = &mixer->channels[mixer->blend_target_channel_index];

    if (target_channel->weight >= 1.0f)
    {
      for (uint32_t channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
      {
        struct AEMAnimationChannel* channel = &mixer->channels[channel_index];
        channel->weight = (channel == target_channel) ? 1.0f : 0.0f;
      }

      mixer->is_blending = false;
    }
    else
    {
      // Store previous progress
      float prev_progress = mixer->blend_progress;

      // Advance blend progress
      {
        const float step = delta_time * mixer->blend_speed;
        mixer->blend_progress = fminf(mixer->blend_progress + step, 1.0f);
      }

      // Eased blend factor
      float prev_eased = prev_progress;
      float eased = mixer->blend_progress;
      if (mixer->blend_mode == AEMAnimationBlendMode_Smooth)
      {
        prev_eased = smoothstep(prev_eased);
        eased = smoothstep(eased);
      }
      else if (mixer->blend_mode == AEMAnimationBlendMode_Smoother)
      {
        prev_eased = smootherstep(prev_eased);
        eased = smootherstep(eased);
      }

      // Interpolate target weight up
      {
        const float initial_weight = mixer->blend_target_channel_initial_weight;
        target_channel->weight = initial_weight + (1.0f - initial_weight) * eased;
      }

      // Reduce others based on eased ratio
      {
        const float fade_ratio = (1.0f - eased) / fmaxf(1.0f - prev_eased, 1e-6f); // Avoid divide by zero

        for (uint32_t channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
        {
          struct AEMAnimationChannel* channel = &mixer->channels[channel_index];
          if (channel != target_channel)
          {
            channel->weight *= fade_ratio;
          }
        }
      }
    }
  }

  mat4* cached_transforms = (mat4*)mixer->joint_transforms;

  for (uint32_t joint_index = 0; joint_index < mixer->joint_count; ++joint_index)
  {
    vec3 t[4], s[4];
    versor r[4];
    for (uint32_t channel_index = 0; channel_index < 4; ++channel_index)
    {
      struct AEMAnimationChannel* channel = &mixer->channels[channel_index];
      get_joint_posed_transform_local_trs(model, joint_index, mixer->channels[channel_index].animation_index,
                                          mixer->channels[channel_index].time, t[channel_index], r[channel_index],
                                          s[channel_index]);
    }

    // Blend a and b
    vec3 t_ab, s_ab;
    versor r_ab;
    const float weight_a = mixer->channels[0].weight;
    const float weight_b = mixer->channels[1].weight;
    {
      float blend = 0.0f;
      if (weight_a + weight_b > 0.0f)
      {
        blend = weight_b / (weight_a + weight_b);
      }

      glm_vec3_lerp(t[0], t[1], blend, t_ab);
      glm_quat_slerp(r[0], r[1], blend, r_ab);
      glm_vec3_lerp(s[0], s[1], blend, s_ab);
    }

    // Blend c and d
    vec3 t_cd, s_cd;
    versor r_cd;
    const float weight_c = mixer->channels[2].weight;
    const float weight_d = mixer->channels[3].weight;
    {
      float blend = 0.0f;
      if (weight_c + weight_d > 0.0f)
      {
        blend = weight_d / (weight_c + weight_d);
      }

      glm_vec3_lerp(t[2], t[3], blend, t_cd);
      glm_quat_slerp(r[2], r[3], blend, r_cd);
      glm_vec3_lerp(s[2], s[3], blend, s_cd);
    }

    // Blend ab and cd
    vec3 blended_t, blended_s;
    versor blended_r;
    {
      const float weight_ab = weight_a + weight_b;
      const float weight_cd = weight_c + weight_d;
      const float blend = weight_cd / (weight_ab + weight_cd);
      glm_vec3_lerp(t_ab, t_cd, blend, blended_t);
      glm_quat_slerp(r_ab, r_cd, blend, blended_r);
      glm_vec3_lerp(s_ab, s_cd, blend, blended_s);
    }

    glm_mat4_identity(cached_transforms[joint_index]);
    glm_translate(cached_transforms[joint_index], blended_t);
    glm_quat_rotate(cached_transforms[joint_index], blended_r, cached_transforms[joint_index]);
    glm_scale(cached_transforms[joint_index], blended_s);
  }

  for (uint32_t joint_index = 0; joint_index < mixer->joint_count; ++joint_index)
  {
    glm_mat4_copy(cached_transforms[joint_index], transforms[joint_index]);

    struct AEMJoint* joint = &model->joints[joint_index];
    int32_t parent_joint_index = joint->parent_joint_index;
    while (parent_joint_index >= 0)
    {
      glm_mat4_mul(cached_transforms[parent_joint_index], transforms[joint_index], transforms[joint_index]);
      parent_joint_index = model->joints[parent_joint_index].parent_joint_index;
    }

    mat4 inverse_bind_matrix;
    glm_mat4_make(joint->inverse_bind_matrix, inverse_bind_matrix);
    glm_mat4_mul(transforms[joint_index], inverse_bind_matrix, transforms[joint_index]);
  }
}