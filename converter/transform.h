#pragma once

#include <cglm/types.h>

#include <stdbool.h>

typedef struct cgltf_node cgltf_node;

void calculate_global_node_transform(cgltf_node* node, mat4 transform);