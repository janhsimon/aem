#pragma once

#include <stdint.h>

typedef struct cgltf_animation cgltf_animation;
typedef struct cgltf_node cgltf_node;

typedef struct Joint Joint;

struct Keyframe
{
  float time; // In seconds
  float data[4];
};

typedef struct Keyframe Keyframe;

uint32_t populate_keyframes(const cgltf_animation* animation, const Joint* joint, Keyframe* keyframes);