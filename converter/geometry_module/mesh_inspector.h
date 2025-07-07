#pragma once

#include <stdbool.h>

typedef struct cgltf_attribute cgltf_attribute;
typedef struct cgltf_data cgltf_data;
typedef struct cgltf_mesh cgltf_mesh;
typedef struct cgltf_node cgltf_node;
typedef struct cgltf_primitive cgltf_primitive;

void locate_attributes_for_primitive(const cgltf_primitive* primitive,
                                     cgltf_attribute** positions,
                                     cgltf_attribute** normals,
                                     cgltf_attribute** tangents,
                                     cgltf_attribute** uvs,
                                     cgltf_attribute** joints,
                                     cgltf_attribute** weights);

cgltf_node* find_node_for_mesh(const cgltf_data* input_file, const cgltf_mesh* mesh);

bool is_primitive_valid(const cgltf_primitive* primitive,
                        const cgltf_attribute* positions,
                        const cgltf_attribute* normals);