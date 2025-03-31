#include "transform.h"

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>

#include <cgltf/cgltf.h>

#include <assert.h>

void calculate_local_node_transform(cgltf_node* node, mat4 transform)
{
  if (node->has_matrix)
  {
    assert(!node->has_translation);
    assert(!node->has_rotation);
    assert(!node->has_scale);

    glm_mat4_make(node->matrix, transform);
    return;
  }

  glm_mat4_identity(transform);

  if (node->has_translation)
  {
    glm_translate(transform, node->translation);
  }

  if (node->has_rotation)
  {
    mat4 rotation;
    glm_quat_mat4(node->rotation, rotation);
    glm_mat4_mul(transform, rotation, transform);
  }

  if (node->has_scale)
  {
    glm_scale(transform, node->scale);
  }
}

void calculate_global_node_transform(cgltf_node* node, mat4 transform)
{
  glm_mat4_identity(transform);

  cgltf_node* n_ptr = node;
  while (n_ptr)
  {
    mat4 local_node_transform;
    calculate_local_node_transform(n_ptr, local_node_transform);

    glm_mat4_mul(local_node_transform, transform, transform);

    n_ptr = n_ptr->parent;
  }
}