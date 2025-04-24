#pragma once

#include <cgltf/cgltf.h>

#include <stdint.h>

struct Animation
{
  const cgltf_animation* animation;
  float duration; // In seconds
  uint32_t sequence_index;
};

typedef struct Animation Animation;

float calculate_animation_duration(const cgltf_animation* animation);

const cgltf_animation_channel* find_channel_for_node(const cgltf_animation* animation,
                                                     const cgltf_node* node,
                                                     const cgltf_animation_path_type type,
                                                     cgltf_size* keyframe_count);