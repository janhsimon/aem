#pragma once

#include <cglm/types.h>

typedef struct cgltf_accessor cgltf_accessor;
typedef struct OutputMesh OutputMesh;

void generate_tangents_with_uvs(OutputMesh* output_mesh,
                                cgltf_accessor* positions,
                                cgltf_accessor* normals,
                                cgltf_accessor* uvs,
                                cgltf_accessor* indices,
                                vec4* reconstructed_tangents);

void generate_tangents_without_uvs(const cgltf_accessor* normals, size_t vertex_count, vec4* tangents);