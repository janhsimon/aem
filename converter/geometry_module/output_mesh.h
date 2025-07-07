#pragma once

#include <cglm/types.h>

#include <stdint.h>

typedef struct cgltf_mesh cgltf_mesh;

struct OutputMesh
{
  cgltf_mesh* input_mesh;

  vec3 *positions, *normals, *tangents, *bitangents;
  vec2* uvs;
  ivec4* joints;
  vec4* weights;

  uint32_t* indices;

  uint64_t vertex_count, index_count, first_index;
  uint32_t material_index;
};

typedef struct OutputMesh OutputMesh;