#pragma once

#include <stdint.h>

typedef struct cgltf_animation cgltf_animation;
typedef struct cgltf_animation_channel cgltf_animation_channel;
typedef struct cgltf_node cgltf_node;

struct Animation
{
  const cgltf_animation* animation;
  float duration; // In seconds
  uint32_t sequence_index;
};

typedef struct Animation Animation;

typedef struct Joint Joint;

float calculate_animation_duration(const cgltf_animation* animation);

void find_animation_channels_for_node(const cgltf_animation* animation,
                                      const cgltf_node* node,
                                      cgltf_animation_channel** translation_channel,
                                      cgltf_animation_channel** rotation_channel,
                                      cgltf_animation_channel** scale_channel);

uint32_t determine_keyframe_count_for_channel(const cgltf_animation_channel* channel);
uint32_t determine_keyframe_count_for_animation(const cgltf_animation* animation, Joint* joints, uint32_t joint_count);
