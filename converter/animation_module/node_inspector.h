#pragma once

#include <cglm/types.h>

#include <stdbool.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_node cgltf_node;
typedef struct cgltf_skin cgltf_skin;

void calculate_local_node_transform(cgltf_node* node, mat4 transform);
void calculate_global_node_transform(cgltf_node* node, mat4 transform);

bool is_node_joint(const cgltf_data* input_file, const cgltf_node* node);
bool is_node_animated(const cgltf_data* input_file, const cgltf_node* node);
bool is_node_mesh(const cgltf_node* node);

cgltf_skin* calculate_skin_for_node(const cgltf_data* input_file, const cgltf_node* node);
cgltf_node* calculate_root_node_for_skin(const cgltf_skin* skin);