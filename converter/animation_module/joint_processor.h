#pragma once

#include <cglm/mat4.h>
#include <cglm/quat.h>

#include <stdint.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_node cgltf_node;

typedef struct AnalyzerNode AnalyzerNode;

struct Joint
{
  AnalyzerNode* node;
  mat4 inverse_bind_matrix;
  int32_t parent_index;

  vec3 pre_transform_translation;
  versor pre_transform_rotation;
  vec3 pre_transform_scale;
};

typedef struct Joint Joint;

void calculate_joint_parent_indices(Joint* joints, uint32_t joint_count);
void calculate_joint_inverse_bind_matrices(const cgltf_data* input_file, Joint* joints, uint32_t joint_count);
void calculate_joint_pre_transforms(Joint* joints, uint32_t joint_count);

int32_t calculate_joint_index_for_node(const cgltf_node* node, Joint* joints, uint32_t joint_count);