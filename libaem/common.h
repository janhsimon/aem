#pragma once

#include "animation_mixer.h"
#include "model.h"

#include <stdbool.h>
#include <stdio.h>

struct Header
{
  uint32_t vertex_count, index_count;
  uint64_t image_buffer_size;
  uint32_t texture_count, mesh_count, material_count;
  uint32_t joint_count, animation_count, track_count, keyframe_count;
  uint32_t padding;
};

struct Vertex
{
  float position[3], normal[3], tangent[3], bitangent[3];
  float uv[2];
  int32_t joint_indices[4];
  float joint_weights[4];
};

struct Animation
{
  aem_string name;
  float duration;
};

struct Track
{
  uint32_t first_keyframe_index;
  uint32_t translation_keyframe_count, rotation_keyframe_count, scale_keyframe_count;
};

struct Keyframe
{
  float time;
  float data[4]; // Pos: [x, y, z, 0], Rot: [x, y, z, w], Scale: [x, y, z, 0]
};

struct AEMModel
{
  FILE* fp; // File pointer that is closed when loading is done
  struct Header header;

  void* load_time_data; // Load-time data that is released when loading is done
  void* run_time_data;  // Run-time data that is kept around after loading is done

  struct Vertex* vertex_buffer;
  uint32_t* index_buffer;
  uint8_t* image_buffer;

  struct AEMTexture* textures;
  struct AEMMesh* meshes;
  struct AEMMaterial* materials;
  struct AEMJoint* joints;
  struct Animation* animations;
  struct Track* tracks;
  struct Keyframe* keyframes;
};

struct AEMAnimationMixer
{
  struct AEMAnimationChannel* channels;
  uint32_t channel_count;

  float* joint_transforms;
  uint32_t joint_count;

  bool is_blending;
  uint32_t blend_target_channel_index;
  float blend_target_channel_initial_weight;
  float blend_progress;
  float blend_speed;
  enum AEMAnimationBlendMode blend_mode;

  bool is_enabled;
};