#include "animation_processor.h"

#include "joint_processor.h"
#include "node_analyzer.h"

#include <cgltf/cgltf.h>

#include <assert.h>

float calculate_animation_duration(const cgltf_animation* animation)
{
  float duration = 0.0f;

  for (cgltf_size sampler_index = 0; sampler_index < animation->samplers_count; ++sampler_index)
  {
    cgltf_animation_sampler* sampler = &animation->samplers[sampler_index];

    float last_sample;
    const cgltf_bool result = cgltf_accessor_read_float(sampler->input, sampler->input->count - 1, &last_sample, 1);
    assert(result);

    if (last_sample > duration)
    {
      duration = last_sample;
    }
  }

  return duration;
}

void find_animation_channels_for_node(const cgltf_animation* animation,
                                      const cgltf_node* node,
                                      cgltf_animation_channel** translation_channel,
                                      cgltf_animation_channel** rotation_channel,
                                      cgltf_animation_channel** scale_channel)
{
  *translation_channel = *rotation_channel = *scale_channel = NULL;

  for (cgltf_size channel_index = 0; channel_index < animation->channels_count; ++channel_index)
  {
    const cgltf_animation_channel* channel = &animation->channels[channel_index];
    if (channel->target_node != node)
    {
      continue;
    }

    if (channel->target_path == cgltf_animation_path_type_translation)
    {
      assert(!*translation_channel);
      *translation_channel = channel;
    }
    else if (channel->target_path == cgltf_animation_path_type_rotation)
    {
      assert(!*rotation_channel);
      *rotation_channel = channel;
    }
    else if (channel->target_path == cgltf_animation_path_type_scale)
    {
      assert(!*scale_channel);
      *scale_channel = channel;
    }
  }
}

uint32_t determine_keyframe_count_for_channel(const cgltf_animation_channel* channel)
{
  if (!channel)
  {
    return 1;
  }

  const cgltf_size keyframe_count = channel->sampler->input->count;
  if (keyframe_count == 0)
  {
    return 1;
  }

  return (uint32_t)keyframe_count;
}

uint32_t determine_keyframe_count_for_animation(const cgltf_animation* animation, Joint* joints, uint32_t joint_count)
{
  uint32_t keyframe_count = 0;
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    const Joint* joint = &joints[joint_index];

    const cgltf_animation_channel *translation_channel, *rotation_channel, *scale_channel;
    find_animation_channels_for_node(animation, joint->node->node, &translation_channel, &rotation_channel,
                                     &scale_channel);

    keyframe_count += determine_keyframe_count_for_channel(translation_channel);
    keyframe_count += determine_keyframe_count_for_channel(rotation_channel);
    keyframe_count += determine_keyframe_count_for_channel(scale_channel);
  }

  return keyframe_count;
}