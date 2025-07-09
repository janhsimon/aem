#include "node_inspector.h"

#include <cgltf/cgltf.h>

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>

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
    vec3 scale;
    glm_vec3_copy(node->scale, scale);

    for (int i = 0; i < 3; ++i)
    {
      if (fabs(scale[i]) < 1e-4f)
      {
        scale[i] = 1e-4f;
      }
    }

    glm_scale(transform, scale);
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

bool is_node_joint(const cgltf_data* input_file, const cgltf_node* node)
{
  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    const cgltf_skin* skin = &input_file->skins[skin_index];
    for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
    {
      const cgltf_node* joint = skin->joints[joint_index];
      if (joint == node)
      {
        return true;
      }
    }
  }

  return false;
}

bool is_node_animated(const cgltf_data* input_file, const cgltf_node* node)
{
  for (cgltf_size animation_index = 0; animation_index < input_file->animations_count; ++animation_index)
  {
    const cgltf_animation* animation = &input_file->animations[animation_index];
    for (cgltf_size channel_index = 0; channel_index < animation->channels_count; ++channel_index)
    {
      const cgltf_animation_channel* channel = &animation->channels[channel_index];
      if (channel->target_node == node)
      {
        return true;
      }
    }
  }

  return false;
}

bool is_node_mesh(const cgltf_node* node)
{
  return node->mesh;
}

cgltf_skin* calculate_skin_for_node(const cgltf_data* input_file, const cgltf_node* node)
{
  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    cgltf_skin* skin = &input_file->skins[skin_index];
    for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
    {
      const cgltf_node* joint = skin->joints[joint_index];
      if (joint == node)
      {
        return skin;
      }
    }
  }

  return NULL;
}

cgltf_node* calculate_root_node_for_skin(const cgltf_skin* skin)
{
  cgltf_node* root = NULL;
  uint32_t min_parent_count;

  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    cgltf_node* joint = skin->joints[joint_index];

    uint32_t parent_count = 0;
    cgltf_node* n_ptr = joint->parent;
    while (n_ptr)
    {
      ++parent_count;
      n_ptr = n_ptr->parent;
    }

    if (!root || parent_count < min_parent_count)
    {
      root = joint;
      min_parent_count = parent_count;
    }
  }

  return root;
}