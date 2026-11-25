#include "joint.h"

#include "analyzer_node.h"
#include "node_inspector.h"

#include <cgltf/cgltf.h>

#include <cglm/affine.h>

#include <assert.h>

void calculate_joint_parent_indices(Joint* joints, uint32_t joint_count)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];

    joint->parent_index = -1;

    AnalyzerNode* n_ptr = joint->analyzer_node->parent;
    while (n_ptr)
    {
      bool parent_found = false;
      for (uint32_t parent_joint_index = 0; parent_joint_index < joint_count; ++parent_joint_index)
      {
        Joint* parent_joint_candidate = &joints[parent_joint_index];
        if (parent_joint_candidate->analyzer_node == n_ptr)
        {
          joint->parent_index = (int32_t)parent_joint_index;
          parent_found = true;
          break;
        }
      }

      if (parent_found)
      {
        break;
      }

      n_ptr = n_ptr->parent;
    }
  }
}

void calculate_joint_inverse_bind_matrices(const cgltf_data* input_file, Joint* joints, uint32_t joint_count)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];

    if (joint->analyzer_node->is_mesh)
    {
      assert(!joint->analyzer_node->is_joint);

      calculate_global_node_transform(joint->analyzer_node->node, joint->inverse_bind_matrix);
      glm_mat4_inv(joint->inverse_bind_matrix, joint->inverse_bind_matrix);
    }
    else if (joint->analyzer_node->is_joint)
    {
      assert(!joint->analyzer_node->is_mesh);

      const cgltf_skin* skin = calculate_skin_for_node(input_file, joint->analyzer_node->node);
      assert(skin);

      // Find the root node in the skin which is sometimes given as skeleton, other times it needs to be manually found
      cgltf_node* skin_root = skin->skeleton;
      if (!skin_root)
      {
        skin_root = calculate_root_node_for_skin(skin);
        assert(skin_root);
      }

      mat4 skin_root_transform; // Transforms from skin to model space (=origin)
      calculate_global_node_transform(skin_root, skin_root_transform);
      glm_mat4_inv(skin_root_transform, skin_root_transform);

      mat4 inv_bind_matrix; // Transforms from joint to skin space
      {
        cgltf_size joint_index_in_skin = 0;
        bool found = false;
        for (cgltf_size i = 0; i < skin->joints_count; ++i)
        {
          if (skin->joints[i] == joint->analyzer_node->node)
          {
            joint_index_in_skin = i;
            found = true;
            break;
          }
        }

        assert(found);
        cgltf_accessor_read_float(skin->inverse_bind_matrices, joint_index_in_skin, (cgltf_float*)inv_bind_matrix, 16);
      }

      // Transform from joint to model space by adding the transform from skin to model space (the inverse of the
      // root's global transform) to the transform from joint to skin space
      glm_mat4_mul(inv_bind_matrix, skin_root_transform, joint->inverse_bind_matrix);
    }
    else
    {
      glm_mat4_identity(joint->inverse_bind_matrix);
    }
  }
}

void calculate_joint_pre_transforms(Joint* joints, uint32_t joint_count)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];

    // Determine the combined transform from all GLB nodes leading up this joint that are not represented as a joint
    glm_mat4_identity(joint->pre_transform);
    {
      AnalyzerNode* n_ptr = joint->analyzer_node->parent; // Starting with the parent
      while (n_ptr && !n_ptr->is_represented)             // For every parent node that is not animated
      {
        mat4 local_transform;
        calculate_local_node_transform(n_ptr->node, local_transform);

        glm_mat4_mul(local_transform, joint->pre_transform, joint->pre_transform);

        n_ptr = n_ptr->parent;
      }
    }
  }
}

int32_t calculate_joint_index_for_node(const cgltf_node* node, Joint* joints, uint32_t joint_count)
{
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    Joint* joint = &joints[joint_index];
    if (joint->analyzer_node->node == node)
    {
      return (int32_t)joint_index;
    }
  }

  return -1;
}