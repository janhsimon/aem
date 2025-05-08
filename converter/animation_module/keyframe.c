#include "keyframe.h"

#include "analyzer_node.h"
#include "animation.h"
#include "animation_sampler.h"
#include "joint.h"

#include <cgltf/cgltf.h>

#include <cglm/quat.h>
#include <cglm/vec3.h>

#include <assert.h>

static void read_values_for_keyframe(const cgltf_animation_channel* channel,
                                     cgltf_size keyframe_index,
                                     cgltf_size element_count,
                                     vec4 out_values,
                                     float* out_time)
{
  // Time
  {
    const cgltf_bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index, out_time, 1);
    assert(result);
  }

  // Data
  {
    glm_vec4_zero(out_values);

    const cgltf_bool result =
      cgltf_accessor_read_float(channel->sampler->output, keyframe_index, out_values, element_count);
    assert(result);
  }
}

static void apply_pre_transform(vec3 translation, versor rotation, vec3 scale, mat4 joint_pre_transform, mat4 out)
{
  // Make a trs matrix
  mat4 trs;
  {
    mat4 translation_matrix;
    glm_translate_make(translation_matrix, translation);

    mat4 rotation_matrix;
    glm_quat_mat4(rotation, rotation_matrix);

    mat4 scale_matrix;
    glm_scale_make(scale_matrix, scale);

    glm_mat4_mul(rotation_matrix, scale_matrix, trs);
    glm_mat4_mul(translation_matrix, trs, trs);
  }

  glm_mat4_mul(joint_pre_transform, trs, out);
}

static void construct_translation_values(const Joint* joint,
                                         const cgltf_animation_channel* translation_channel,
                                         float time,
                                         vec4 out_values)
{
  if (!translation_channel || translation_channel->sampler->input->count == 0)
  {
    glm_vec4_zero(out_values);

    const cgltf_node* node = joint->analyzer_node->node;
    if (node->has_translation)
    {
      glm_vec3_copy(node->translation, out_values);
    }
  }
  else
  {
    determine_interpolated_values_for_keyframe_at(translation_channel, time, 3, out_values);
  }
}

static void construct_rotation_values(const Joint* joint,
                                      const cgltf_animation_channel* rotation_channel,
                                      float time,
                                      vec4 out_values)
{
  if (!rotation_channel || rotation_channel->sampler->input->count == 0)
  {
    const cgltf_node* node = joint->analyzer_node->node;
    if (node->has_rotation)
    {
      glm_quat_copy(node->rotation, out_values);
    }
    else
    {
      glm_quat_identity(out_values);
    }
  }
  else
  {
    determine_interpolated_values_for_keyframe_at(rotation_channel, time, 4, out_values);
    glm_quat_normalize(out_values);
  }
}

static void
construct_scale_values(const Joint* joint, const cgltf_animation_channel* scale_channel, float time, vec4 out_values)
{
  if (!scale_channel || scale_channel->sampler->input->count == 0)
  {
    glm_vec4_one(out_values);

    const cgltf_node* node = joint->analyzer_node->node;
    if (node->has_scale)
    {
      glm_vec3_copy(node->scale, out_values);
    }
  }
  else
  {
    determine_interpolated_values_for_keyframe_at(scale_channel, time, 3, out_values);
  }
}

static void correct_translation_values(const Joint* joint,
                                       vec3 translation,
                                       const cgltf_animation_channel* rotation_channel,
                                       const cgltf_animation_channel* scale_channel,
                                       float time,
                                       vec4 out_values)
{
  versor constructed_rotation_values;
  construct_rotation_values(joint, rotation_channel, time, constructed_rotation_values);

  vec4 constructed_scale_values;
  construct_scale_values(joint, scale_channel, time, constructed_scale_values);

  mat4 corrected;
  apply_pre_transform(translation, constructed_rotation_values, constructed_scale_values, joint->pre_transform,
                      corrected);

  mat4 unused_rotation;
  vec3 unused_scale;
  glm_decompose(corrected, out_values, unused_rotation, unused_scale);

  out_values[3] = 0.0f;
}

static void correct_rotation_values(const Joint* joint,
                                    versor rotation,
                                    const cgltf_animation_channel* translation_channel,
                                    const cgltf_animation_channel* scale_channel,
                                    float time,
                                    vec4 out_values)
{
  vec4 constructed_translation_values;
  construct_translation_values(joint, translation_channel, time, constructed_translation_values);

  vec4 constructed_scale_values;
  construct_scale_values(joint, scale_channel, time, constructed_scale_values);

  mat4 corrected;
  apply_pre_transform(constructed_translation_values, rotation, constructed_scale_values, joint->pre_transform,
                      corrected);

  vec4 unused_translation;
  mat4 rotation_matrix;
  vec3 unused_scale;
  glm_decompose(corrected, unused_translation, rotation_matrix, unused_scale);

  glm_mat4_quat(rotation_matrix, out_values); // Convert rotation matrix to quaternion
  glm_quat_normalize(out_values);
}

static void correct_scale_values(const Joint* joint,
                                 vec3 scale,
                                 const cgltf_animation_channel* translation_channel,
                                 const cgltf_animation_channel* rotation_channel,
                                 float time,
                                 vec4 out_values)
{
  vec4 constructed_translation_values;
  construct_translation_values(joint, translation_channel, time, constructed_translation_values);

  versor constructed_rotation_values;
  construct_rotation_values(joint, rotation_channel, time, constructed_rotation_values);

  mat4 corrected;
  apply_pre_transform(constructed_translation_values, constructed_rotation_values, scale, joint->pre_transform,
                      corrected);

  vec4 unused_translation;
  mat4 unused_rotation;
  glm_decompose(corrected, unused_translation, unused_rotation, out_values);

  out_values[3] = 0.0f;
}

uint32_t populate_keyframes(const cgltf_animation* animation, const Joint* joint, Keyframe* keyframes)
{
  uint32_t keyframes_written = 0;

  const cgltf_node* node = joint->analyzer_node->node;

  cgltf_animation_channel *translation_channel, *rotation_channel, *scale_channel;
  find_animation_channels_for_node(animation, node, &translation_channel, &rotation_channel, &scale_channel);

  // Translation
  if (!translation_channel || translation_channel->sampler->input->count == 0)
  {
    vec3 translation = GLM_VEC3_ZERO_INIT;
    if (node->has_translation)
    {
      glm_vec3_copy(node->translation, translation);
    }

    Keyframe* keyframe = &keyframes[keyframes_written];
    keyframe->time = 0.0f;
    correct_translation_values(joint, translation, rotation_channel, scale_channel, 0.0f, keyframe->data);

    ++keyframes_written;
  }
  else
  {
    for (cgltf_size keyframe_index = 0; keyframe_index < translation_channel->sampler->input->count; ++keyframe_index)
    {
      vec4 translation_values;
      float time;
      read_values_for_keyframe(translation_channel, keyframe_index, 3, translation_values, &time);

      vec3 translation;
      glm_vec4_copy3(translation_values, translation);

      Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
      keyframe->time = time;
      correct_translation_values(joint, translation, rotation_channel, scale_channel, time, keyframe->data);
    }

    keyframes_written += (uint32_t)(translation_channel->sampler->input->count);
  }

  // Rotation
  if (!rotation_channel || rotation_channel->sampler->input->count == 0)
  {
    versor rotation = GLM_QUAT_IDENTITY_INIT;
    if (node->has_rotation)
    {
      glm_quat_copy(node->rotation, rotation);
    }

    Keyframe* keyframe = &keyframes[keyframes_written];
    keyframe->time = 0.0f;
    correct_rotation_values(joint, rotation, translation_channel, scale_channel, 0.0f, keyframe->data);

    ++keyframes_written;
  }
  else
  {
    for (cgltf_size keyframe_index = 0; keyframe_index < rotation_channel->sampler->input->count; ++keyframe_index)
    {
      versor rotation;
      float time;
      read_values_for_keyframe(rotation_channel, keyframe_index, 4, rotation, &time);
      glm_quat_normalize(rotation);

      Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
      keyframe->time = time;
      correct_rotation_values(joint, rotation, translation_channel, scale_channel, time, keyframe->data);
    }

    keyframes_written += (uint32_t)(rotation_channel->sampler->input->count);
  }

  // Scale
  if (!scale_channel || scale_channel->sampler->input->count == 0)
  {
    vec3 scale = GLM_VEC3_ONE_INIT;
    if (node->has_scale)
    {
      glm_vec3_copy(node->scale, scale);
    }

    Keyframe* keyframe = &keyframes[keyframes_written];
    keyframe->time = 0.0f;
    correct_scale_values(joint, scale, translation_channel, rotation_channel, 0.0f, keyframe->data);

    ++keyframes_written;
  }
  else
  {
    for (cgltf_size keyframe_index = 0; keyframe_index < scale_channel->sampler->input->count; ++keyframe_index)
    {
      vec4 scale_values;
      float time;
      read_values_for_keyframe(scale_channel, keyframe_index, 3, scale_values, &time);

      vec3 scale;
      glm_vec4_copy3(scale_values, scale);

      Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
      keyframe->time = time;
      correct_scale_values(joint, scale, translation_channel, rotation_channel, time, keyframe->data);
    }

    keyframes_written += (uint32_t)(scale_channel->sampler->input->count);
  }

  return keyframes_written;
}