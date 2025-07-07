#pragma once

#include <cglm/types.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_accessor cgltf_accessor;
typedef struct OutputMesh OutputMesh;

void reconstruct_tangents(const cgltf_data* input_file,
                          OutputMesh* output_mesh,
                          cgltf_accessor* positions,
                          cgltf_accessor* normals,
                          cgltf_accessor* uvs,
                          cgltf_accessor* indices,
                          vec4* original_tangents,
                          vec4* reconstructed_tangents);