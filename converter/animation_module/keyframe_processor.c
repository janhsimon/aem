#include "keyframe_processor.h"

#include "animation_processor.h"
#include "joint_processor.h"
#include "node_analyzer.h"

#include <cgltf/cgltf.h>

#include <cglm/quat.h>
#include <cglm/vec3.h>

#include <assert.h>

// static void
// populate_keyframes_for_channel(const cgltf_animation_channel* channel, cgltf_size element_count, Keyframe* keyframes)
//{
//   const cgltf_size keyframe_count = channel->sampler->input->count;
//   for (cgltf_size keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
//   {
//     Keyframe* keyframe = &keyframes[keyframe_index];
//
//     // Time
//     {
//       const cgltf_bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &keyframe->time,
//       1); assert(result);
//     }
//
//     // Data
//     {
//       const cgltf_bool result =
//         cgltf_accessor_read_float(channel->sampler->output, keyframe_index, &keyframe->data[0], element_count);
//       assert(result);
//     }
//
//     // Apply pre transform to data
//     {
//     }
//   }
// }

static void set_keyframe_from_channel(Keyframe* keyframe,
                                      cgltf_size keyframe_index,
                                      const cgltf_animation_channel* channel,
                                      cgltf_size element_count)
{
  // Time
  {
    const cgltf_bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &keyframe->time, 1);
    assert(result);
  }

  // Data
  {
    glm_vec4_zero(keyframe->data);

    const cgltf_bool result =
      cgltf_accessor_read_float(channel->sampler->output, keyframe_index, &keyframe->data[0], element_count);
    assert(result);
  }
}

static void set_single_keyframe(Keyframe* keyframe, vec4 values)
{
  keyframe->time = 0.0f;
  glm_vec4_copy(values, keyframe->data);
}

static void apply_pre_translation(Keyframe* keyframe, const Joint* joint)
{
  glm_vec3_add(joint->pre_transform_translation, keyframe->data, keyframe->data);
}

static void apply_pre_rotation(Keyframe* keyframe, const Joint* joint)
{
  glm_quat_mul(joint->pre_transform_rotation, keyframe->data, keyframe->data);
}

static void apply_pre_scale(Keyframe* keyframe, const Joint* joint)
{
  glm_vec3_mul(joint->pre_transform_scale, keyframe->data, keyframe->data);
}

uint32_t populate_keyframes(const cgltf_animation* animation, const Joint* joint, Keyframe* keyframes)
{
  uint32_t keyframes_written = 0;

  const cgltf_node* node = joint->node->node;

  // Translation
  {
    cgltf_size translation_keyframe_count;
    const cgltf_animation_channel* channel =
      find_channel_for_node(animation, node, cgltf_animation_path_type_translation, &translation_keyframe_count);
    if (channel && translation_keyframe_count > 0)
    {
      for (cgltf_size keyframe_index = 0; keyframe_index < translation_keyframe_count; ++keyframe_index)
      {
        Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
        set_keyframe_from_channel(keyframe, keyframe_index, channel, 3);
        apply_pre_translation(keyframe, joint);
      }

      keyframes_written += (uint32_t)translation_keyframe_count;
    }
    else
    {
      vec4 translation = GLM_VEC4_ZERO_INIT;
      if (node->has_translation)
      {
        glm_vec3_copy(node->translation, translation);
      }

      Keyframe* keyframe = &keyframes[keyframes_written];
      set_single_keyframe(keyframe, translation);
      apply_pre_translation(keyframe, joint);

      ++keyframes_written;
    }
  }

  // Rotation
  {
    cgltf_size rotation_keyframe_count;
    const cgltf_animation_channel* channel =
      find_channel_for_node(animation, node, cgltf_animation_path_type_rotation, &rotation_keyframe_count);
    if (channel && rotation_keyframe_count > 0)
    {
      for (cgltf_size keyframe_index = 0; keyframe_index < rotation_keyframe_count; ++keyframe_index)
      {
        Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
        set_keyframe_from_channel(keyframe, keyframe_index, channel, 4);
        apply_pre_rotation(keyframe, joint);
      }

      keyframes_written += (uint32_t)rotation_keyframe_count;
    }
    else
    {
      versor rotation = GLM_QUAT_IDENTITY_INIT;
      if (node->has_rotation)
      {
        glm_quat_copy(node->rotation, rotation);
      }

      Keyframe* keyframe = &keyframes[keyframes_written];
      set_single_keyframe(keyframe, rotation);
      apply_pre_rotation(keyframe, joint);

      ++keyframes_written;
    }
  }

  // Scale
  {
    cgltf_size scale_keyframe_count;
    const cgltf_animation_channel* channel =
      find_channel_for_node(animation, node, cgltf_animation_path_type_scale, &scale_keyframe_count);
    if (channel && scale_keyframe_count > 0)
    {
      for (cgltf_size keyframe_index = 0; keyframe_index < scale_keyframe_count; ++keyframe_index)
      {
        Keyframe* keyframe = &keyframes[keyframes_written + keyframe_index];
        set_keyframe_from_channel(keyframe, keyframe_index, channel, 3);
        apply_pre_scale(keyframe, joint);
      }

      keyframes_written += (uint32_t)scale_keyframe_count;
    }
    else
    {
      vec4 scale = GLM_VEC4_ONE_INIT;
      if (node->has_scale)
      {
        glm_vec3_copy(node->scale, scale);
      }

      Keyframe* keyframe = &keyframes[keyframes_written];
      set_single_keyframe(keyframe, scale);
      apply_pre_scale(keyframe, joint);

      ++keyframes_written;
    }
  }

  return keyframes_written;
}