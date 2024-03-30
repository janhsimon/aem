#pragma once

#include <stdio.h>

struct Header
{
  uint32_t vertex_count, index_count, mesh_count, material_count, texture_count, bone_count, animation_count,
    keyframe_count;
};

struct AEMModel
{
  FILE* fp; // File pointer that is closed when loading is done
  struct Header header;
  void* load_time_data; // Load-time data that is released when loading is done
  void* run_time_data;

  // Vertex format:
  /*
  {
    float position[3], normal[3], tangent[3], bitangent[3];
    float uv[2];
    int32_t bone_indices[4];
    float bone_weights[4];
    int32_t extra_bone_index;
  }
  */

  // Load-time data format:
  /*
  {
    vertices,
    indices,
    texture_filenames
  }
  */

  // Animation format:
  /*
  {
    aem_string name;
    float duration;
    uint32_t first_position_keyframe_index, position_keyframe_count;
    uint32_t first_rotation_keyframe_index, rotation_keyframe_count;
    uint32_t first_scale_keyframe_index, scale_keyframe_count;
  }
  */

  // Run-time data format:
  /*
  {
    meshes,
    materials,
    bones,
    animations,
    position_keyframes,
    rotation_keyframes,
    scale_keyframes
  }
  */
};

struct Keyframe
{
  float time;
  float data[4]; // Pos: [x, y, z, 0], Rot: [x, y, z, w], Scale: [x, y, z, 0]
};