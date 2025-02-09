#pragma once

#include <stdio.h>

struct Header
{
  uint64_t vertex_buffer_size, index_buffer_size, image_buffer_size;
  uint32_t level_count, texture_count, mesh_count, material_count;
  uint32_t bone_count, animation_count, sequence_count, keyframe_count;
};

struct Vertex
{
  float position[3], normal[3], tangent[3], bitangent[3];
  float uv[2];
  int32_t bone_indices[4];
  float bone_weights[4];
  int32_t extra_bone_index;
};

struct Animation
{
  aem_string name;
  float duration;
  uint32_t sequence_index;
};

struct Sequence
{
  uint32_t first_position_keyframe_index, position_keyframe_count;
  uint32_t first_rotation_keyframe_index, rotation_keyframe_count;
  uint32_t first_scale_keyframe_index, scale_keyframe_count;
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

  struct AEMLevel* levels;
  struct AEMTexture* textures;
  struct AEMMesh* meshes;
  struct AEMMaterial* materials;
  struct AEMBone* bones;
  struct Animation* animations;
  struct Sequence* sequences;
  struct Keyframe* keyframes;
};

struct AEMTextureData
{
  uint32_t base_width, base_height;
  uint32_t mip_level_count;
  uint64_t* mip_offsets; // Offsets into data, one offets per mip level, in bytes
  uint64_t* mip_sizes;   // One size per mip level, in bytes

  void* data;
};