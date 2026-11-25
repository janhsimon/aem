#pragma once

#include <cglm/types.h>

#include <stdbool.h>

typedef struct cgltf_texture_transform cgltf_texture_transform;
typedef struct cgltf_texture_view cgltf_texture_view;

void make_identity_transform(cgltf_texture_transform* transform);
void copy_transforms(const cgltf_texture_transform* from, cgltf_texture_transform* to);
cgltf_texture_transform make_transform_for_texture_view(const cgltf_texture_view* texture_view);
bool compare_transforms(cgltf_texture_transform* a, cgltf_texture_transform* b);
bool is_transform_identity(cgltf_texture_transform* transform);
void transform_to_mat3(cgltf_texture_transform* transform, mat3 matrix);