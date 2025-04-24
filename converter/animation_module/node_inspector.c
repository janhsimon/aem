#include "node_inspector.h"

#include <cgltf/cgltf.h>

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
    const cgltf_skin* skin = &input_file->skins[skin_index];
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
    const cgltf_node* joint = skin->joints[joint_index];

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