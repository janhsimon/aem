#include "animation_sampler.h"

#include <cgltf/cgltf.h>

#include <cglm/quat.h>
#include <cglm/vec3.h>

#include <assert.h>
#include <stdbool.h>

enum TimeRange
{
  TimeRange_Before,
  TimeRange_After
};

typedef enum TimeRange TimeRange;

static float determine_keyframe_time(const cgltf_animation_channel* channel, cgltf_size keyframe_index)
{
  float time;
  const bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index, &time, 1);
  assert(result);
  return time;
}

static cgltf_size find_closest_keyframe_in_channel(const cgltf_animation_channel* channel, float time, TimeRange range)
{
  const cgltf_size keyframe_count = channel->sampler->input->count;
  assert(keyframe_count > 0);

  if (keyframe_count == 1)
  {
    return 0;
  }

  if (range == TimeRange_Before)
  {
    for (cgltf_size keyframe_index = 0; keyframe_index < keyframe_count - 1; ++keyframe_index)
    {
      if (determine_keyframe_time(channel, keyframe_index + 1) > time)
      {
        return keyframe_index;
      }
    }

    return keyframe_count - 1;
  }
  else if (range == TimeRange_After)
  {
    for (cgltf_size keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
    {
      if (determine_keyframe_time(channel, keyframe_index) > time)
      {
        return keyframe_index;
      }
    }

    return keyframe_count - 1;
  }

  assert(false);
  return 0;
}

void determine_interpolated_values_for_keyframe_at(const cgltf_animation_channel* channel,
                                                   float time,
                                                   int element_count,
                                                   float out_values[4])
{
  cgltf_size keyframe_index_before = find_closest_keyframe_in_channel(channel, time, TimeRange_Before);
  cgltf_size keyframe_index_after = find_closest_keyframe_in_channel(channel, time, TimeRange_After);

  if (keyframe_index_before == keyframe_index_after)
  {
    // No interpolation needed
    const bool result =
      cgltf_accessor_read_float(channel->sampler->output, keyframe_index_before, out_values, element_count);
    assert(result);

    return;
  }

  float keyframe_time_before, keyframe_time_after;
  vec4 keyframe_values_before, keyframe_values_after;

  // Before
  {
    // Time
    bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index_before, &keyframe_time_before, 1);
    assert(result);

    // Values
    result =
      cgltf_accessor_read_float(channel->sampler->output, keyframe_index_before, keyframe_values_before, element_count);
    assert(result);
  }

  // After
  {
    // Time
    bool result = cgltf_accessor_read_float(channel->sampler->input, keyframe_index_after, &keyframe_time_after, 1);
    assert(result);

    // Values
    result =
      cgltf_accessor_read_float(channel->sampler->output, keyframe_index_after, keyframe_values_after, element_count);
    assert(result);
  }

  const float t = (time - keyframe_time_before) / (keyframe_time_after - keyframe_time_before);
  if (element_count == 3)
  {
    glm_vec3_lerp(keyframe_values_before, keyframe_values_after, t, out_values);
  }
  else if (element_count == 4)
  {
    glm_quat_slerp(keyframe_values_before, keyframe_values_after, t, out_values);
    glm_quat_normalize(out_values);
  }
}