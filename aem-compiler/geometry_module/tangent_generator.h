#pragma once

typedef struct cgltf_accessor cgltf_accessor;
typedef struct OutputMesh OutputMesh;

void generate_tangents_with_uvs(const OutputMesh* output_mesh,
                                const cgltf_accessor* positions,
                                const cgltf_accessor* normals,
                                const cgltf_accessor* uvs,
                                const cgltf_accessor* indices);

void generate_tangents_without_uvs(const OutputMesh* output_mesh, const cgltf_accessor* normals);

void generate_tangents_from_normals_tangents(const OutputMesh* output_mesh,
                                             const cgltf_accessor* normals,
                                             const cgltf_accessor* tangents);
