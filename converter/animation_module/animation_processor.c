#include "animation_processor.h"

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

const cgltf_animation_channel* find_channel_for_node(const cgltf_animation* animation,
                                                     const cgltf_node* node,
                                                     const cgltf_animation_path_type type,
                                                     cgltf_size* keyframe_count)
{
  for (cgltf_size channel_index = 0; channel_index < animation->channels_count; ++channel_index)
  {
    const cgltf_animation_channel* channel = &animation->channels[channel_index];

    if (channel->target_node == node && channel->target_path == type)
    {
      *keyframe_count = channel->sampler->input->count;
      return channel;
    }
  }

  *keyframe_count = 0;
  return NULL;
}