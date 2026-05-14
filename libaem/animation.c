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

static uint32_t
get_joint_transform_index(const struct AEMAnimationMixer* mixer, uint32_t layer_index, uint32_t joint_index)
{
  return layer_index * mixer->joint_count + joint_index;
}

enum AEMAnimationMixerResult aem_load_animation_mixer(uint32_t joint_count,
                                                      uint32_t channel_count,
                                                      uint32_t layer_count,
                                                      struct AEMAnimationMixer** mixer)
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

  (*mixer)->joint_transforms = malloc(sizeof(mat4) * joint_count * layer_count);
  if (!(*mixer)->joint_transforms)
  {
    return AEMAnimationMixerResult_OutOfMemory;
  }

  (*mixer)->channel_count = channel_count;
  (*mixer)->joint_count = joint_count;
  (*mixer)->layer_count = layer_count;

  // Initialize all joint transforms for potential additive layers to identity
  {
    mat4* joint_transforms = (mat4*)(*mixer)->joint_transforms;

    for (uint32_t layer_index = 1; layer_index < layer_count; ++layer_index)
    {
      for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
      {
        glm_mat4_identity(joint_transforms[get_joint_transform_index(*mixer, layer_index, joint_index)]);
      }
    }
  }

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

static calc_joint_to_model_transform(const struct AEMModel* model,
                                     const struct AEMAnimationMixer* mixer,
                                     uint32_t joint_index,
                                     mat4 joint_to_model_transform)
{
  mat4* joint_transforms = (mat4*)mixer->joint_transforms;

  glm_mat4_identity(joint_to_model_transform);

  int32_t parent_joint_index = model->joints[joint_index].parent_joint_index;
  while (parent_joint_index >= 0)
  {
    mat4 combined_joint_transform;

    // Base layer
    glm_mat4_copy(joint_transforms[parent_joint_index], combined_joint_transform);

    // Add additive layers
    for (uint32_t layer_index = 1; layer_index < mixer->layer_count; ++layer_index)
    {
      glm_mat4_mul(combined_joint_transform,
                   joint_transforms[get_joint_transform_index(mixer, layer_index, parent_joint_index)],
                   combined_joint_transform);
    }

    glm_mat4_mul(combined_joint_transform, joint_to_model_transform, joint_to_model_transform);

    parent_joint_index = model->joints[parent_joint_index].parent_joint_index;
  }
}

static uint32_t layer_index_to_bitmask(uint32_t layer_index)
{
  return (1 << layer_index);
}

static bool layers_are_index(enum AEMAnimationLayer layers, uint32_t layer_index)
{
  return layer_index_to_bitmask(layer_index) == layers;
}

static bool layers_include_index(enum AEMAnimationLayer layers, uint32_t layer_index)
{
  return layer_index_to_bitmask(layer_index) & layers;
}

void aem_get_animation_mixer_joint_transform(const struct AEMModel* model,
                                             const struct AEMAnimationMixer* mixer,
                                             uint32_t joint_index,
                                             enum AEMAnimationLayer layers,
                                             enum AEMJointTransformSpace space,
                                             float transform[16])
{
  if (!mixer->is_enabled)
  {
    struct AEMJoint* joint = &model->joints[joint_index];

    mat4 bind_matrix;
    glm_mat4_make(joint->inverse_bind_matrix, bind_matrix);
    glm_mat4_inv(bind_matrix, (vec4*)transform);
    return;
  }

  mat4* joint_transforms = (mat4*)mixer->joint_transforms;

  // If a single layer was selected
  bool done = false;
  for (uint32_t layer_index = 0; layer_index < mixer->layer_count; ++layer_index)
  {
    if (layers_are_index(layers, layer_index))
    {
      glm_mat4_copy(joint_transforms[get_joint_transform_index(mixer, layer_index, joint_index)], (vec4*)transform);
      done = true;
      break;
    }
  }

  // Otherwise add multiple layers together
  if (!done)
  {
    if (layers & AEMAnimationLayer_Base)
    {
      // Base layer
      glm_mat4_copy(joint_transforms[get_joint_transform_index(mixer, 0, joint_index)], (vec4*)transform);

      // Add additive layers
      for (uint32_t layer_index = 1; layer_index < mixer->layer_count; ++layer_index)
      {
        if (layers_include_index(layers, layer_index))
        {
          glm_mat4_mul((vec4*)transform, joint_transforms[get_joint_transform_index(mixer, layer_index, joint_index)],
                       (vec4*)transform);
        }
      }
    }
  }

  if (space == AEMJointTransformSpace_Global)
  {
    // Get the joint to model transform (this takes offsets into account)
    mat4 joint_to_model;
    calc_joint_to_model_transform(model, mixer, joint_index, joint_to_model);

    // Bring the local joint-space transform to model space
    glm_mat4_mul(joint_to_model, (vec4*)transform, (vec4*)transform);
  }
}

void aem_set_animation_mixer_joint_transform(const struct AEMModel* model,
                                             struct AEMAnimationMixer* mixer,
                                             uint32_t joint_index,
                                             enum AEMAnimationLayer layer,
                                             enum AEMJointTransformSpace space,
                                             float transform[16])
{
  mat4* joint_transforms = (mat4*)mixer->joint_transforms;

  for (uint32_t layer_index = 0; layer_index < mixer->layer_count; ++layer_index)
  {
    if (layers_are_index(layer, layer_index))
    {
      const struct AEMJoint* joint = &model->joints[joint_index];

      // Correct coordinate system for global transforms
      if (space == AEMJointTransformSpace_Global && joint->parent_joint_index >= 0)
      {
        mat4 parent_transform;
        calc_joint_to_model_transform(model, mixer, joint_index, parent_transform);

        mat3 parent_rot;
        glm_mat4_pick3(parent_transform, parent_rot);

        // Remove scale from basis vectors
        glm_vec3_normalize(parent_rot[0]);
        glm_vec3_normalize(parent_rot[1]);
        glm_vec3_normalize(parent_rot[2]);

        mat3 inv_parent_rot;
        glm_mat3_transpose_to(parent_rot, inv_parent_rot);

        mat4 parent4 = GLM_MAT4_IDENTITY_INIT;
        mat4 inv4 = GLM_MAT4_IDENTITY_INIT;
        glm_mat4_ins3(parent_rot, parent4);
        glm_mat4_ins3(inv_parent_rot, inv4);

        // local = inv(parent) * global * parent
        mat4 temp4;
        glm_mat4_mul(inv4, (vec4*)transform, temp4);
        glm_mat4_mul(temp4, parent4, (vec4*)transform);
      }

      glm_mat4_copy((vec4*)transform, joint_transforms[get_joint_transform_index(mixer, layer_index, joint_index)]);

      return;
    }
  }
}

uint32_t aem_get_free_animation_mixer_channel_index(const struct AEMAnimationMixer* mixer)
{
  float min_weight = 1.0f;
  uint32_t least_significant_channel_index = 0;
  for (uint32_t channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
  {
    struct AEMAnimationChannel* channel = &mixer->channels[channel_index];
    if (channel->weight <= 0.0f)
    {
      return channel_index;
    }

    if (channel->weight < min_weight)
    {
      least_significant_channel_index = channel_index;
      min_weight = channel->weight;
    }
  }

  return least_significant_channel_index;
}

void aem_cut_to_animation_mixer_channel(struct AEMAnimationMixer* mixer, uint32_t channel_index_)
{
  mixer->is_blending = false;

  for (uint32_t channel_index = 0; channel_index < mixer->channel_count; ++channel_index)
  {
    struct AEMAnimationChannel* channel = &mixer->channels[channel_index];
    channel->weight = (channel_index == channel_index_);
  }
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
                          float* joint_transforms_)
{
  mat4* output_transforms = (mat4*)joint_transforms_;

  // Show the bind pose and early out if the mixer is not enabled
  if (!mixer->is_enabled)
  {
    for (uint32_t joint_index = 0; joint_index < mixer->joint_count; ++joint_index)
    {
      glm_mat4_identity(output_transforms[joint_index]);
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

  mat4* joint_transforms = (mat4*)mixer->joint_transforms;

  // Pass 1: Update the base pose in jointspace, apply joint-space additive offset
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

    // Base layer
    const uint32_t base_index = get_joint_transform_index(mixer, 0, joint_index);
    glm_mat4_identity(joint_transforms[base_index]);
    glm_translate(joint_transforms[base_index], blended_t);
    glm_quat_rotate(joint_transforms[base_index], blended_r, joint_transforms[base_index]);
    glm_scale(joint_transforms[base_index], blended_s);

    if (mixer->layer_count == 1)
    {
      glm_mat4_copy(joint_transforms[base_index], output_transforms[base_index]);
    }
    else
    {
      // Add additive layers
      for (uint32_t layer_index = 1; layer_index < mixer->layer_count; ++layer_index)
      {
        glm_mat4_mul(joint_transforms[base_index],
                     joint_transforms[get_joint_transform_index(mixer, layer_index, joint_index)],
                     output_transforms[joint_index]);
      }
    }
  }

  // Pass 2: Convert outcome of the first pass from joint- to modelspace, use the inverse bind matrix to calculate the
  // final delta output for skeletal animation of the joints
  for (uint32_t joint_index = 0; joint_index < mixer->joint_count; ++joint_index)
  {
    struct AEMJoint* joint = &model->joints[joint_index];

    // Get the joint to model transform (this takes offsets into account)
    mat4 joint_to_model;
    calc_joint_to_model_transform(model, mixer, joint_index, joint_to_model);

    // Convert the combined joint transform from joint to model space
    glm_mat4_mul(joint_to_model, output_transforms[joint_index], output_transforms[joint_index]);

    // Use the inverse bind matrix to evaluate the final output transform for this joint
    mat4 inverse_bind_matrix;
    glm_mat4_make(joint->inverse_bind_matrix, inverse_bind_matrix);
    glm_mat4_mul(output_transforms[joint_index], inverse_bind_matrix, output_transforms[joint_index]);
  }
}