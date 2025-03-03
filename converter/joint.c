#include "joint.h"

#include "config.h"
#include "transform.h"

#include <aem/aem.h>

#include <cgltf/cgltf.h>

#include <cglm/io.h>
#include <cglm/mat4.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  cgltf_node* input_joint;
  mat4 inverse_bind_matrix;
  int32_t parent_index;
} OutputJoint;

static cgltf_size output_joint_count;
static OutputJoint* output_joints;

static bool is_node_in_skin(const cgltf_node* node, const cgltf_skin* skin)
{
  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    const cgltf_node* joint = skin->joints[joint_index];
    if (joint == node)
    {
      return true;
    }
  }

  return false;
}

static void print_node(const cgltf_data* input_file, const cgltf_node* node, uint32_t indentation)
{
  for (uint32_t indent = 1; indent < indentation; ++indent)
  {
    printf("| ");
  }

  if (indentation > 0)
  {
    printf("|-");
  }

  printf("%s", node->name);

  if (node->mesh)
  {
    printf(" [MESH: \"%s\"]", node->mesh->name);
  }

  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    const cgltf_skin* skin = &input_file->skins[skin_index];
    if (is_node_in_skin(node, skin))
    {
      printf(" [SKIN #%llu]", skin_index);
      // Do not break here to show that a node can be in multiple skins
    }

    if (skin->skeleton == node)
    {
      printf(" [SKIN #%llu ROOT]", skin_index);
    }
  }

  if (node->has_matrix)
  {
    printf(" [MATRIX]");
  }

  printf("\n");

  indentation += 1;

  for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
  {
    print_node(input_file, node->children[child_index], indentation);
  }
}

static int32_t calculate_input_joint_index(const cgltf_skin* skin, cgltf_node* joint)
{
  for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
  {
    if (skin->joints[joint_index] == joint)
    {
      return joint_index;
    }
  }

  return -1;
}

static int32_t calculate_output_joint_index(cgltf_node* joint)
{
  if (!joint)
  {
    return -1;
  }

  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    if (output_joints[output_joint_index].input_joint == joint)
    {
      return output_joint_index;
    }
  }

  return -1;
}

static bool is_joint_referenced(const cgltf_data* input_file, cgltf_size joint_index)
{
  for (cgltf_size mesh_index = 0; mesh_index < input_file->meshes_count; ++mesh_index)
  {
    const cgltf_mesh* mesh = &input_file->meshes[mesh_index];

    for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
      const cgltf_primitive* primitive = &mesh->primitives[primitive_index];

      if (primitive->type != cgltf_primitive_type_triangles)
      {
        continue;
      }

      for (cgltf_size attribute_index = 0; attribute_index < primitive->attributes_count; ++attribute_index)
      {
        const cgltf_attribute* attribute = &primitive->attributes[attribute_index];

        if (attribute->type != cgltf_attribute_type_joints)
        {
          continue;
        }

        const cgltf_size vertex_count = attribute->data->count;
        for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
        {
          cgltf_uint joint_indices[4];
          const bool result = cgltf_accessor_read_uint(attribute->data, vertex_index, joint_indices, 4);
          assert(result);

          for (cgltf_size i = 0; i < 4; ++i)
          {
            if ((cgltf_size)joint_indices[i] == joint_index)
            {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

static bool is_input_joint_unnecessary(const cgltf_data* input_file,
                                       const cgltf_skin* skin,
                                       cgltf_node* joint,
                                       bool* joint_referenced)
{
  const int32_t joint_index = calculate_input_joint_index(skin, joint);
  if (joint_index < 0)
  {
    return true;
  }

  if (joint_referenced[joint_index])
  {
    return false;
  }

  for (cgltf_size child_index = 0; child_index < joint->children_count; ++child_index)
  {
    cgltf_node* child = joint->children[child_index];
    if (!is_input_joint_unnecessary(input_file, skin, child, joint_referenced))
    {
      return false;
    }
  }

  return true;
}

void setup_joint_output(const cgltf_data* input_file)
{
  output_joint_count = 0;
  output_joints = NULL;

  if (input_file->skins_count == 0)
  {
    return;
  }

#ifdef PRINT_NODES
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    const cgltf_node* node = &input_file->nodes[node_index];
    if (!node->parent)
    {
      print_node(input_file, node, 0);
    }
  }
#endif

  // Count input joints
  cgltf_size input_joint_count = 0;
  {
    for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
    {
      const cgltf_skin* skin = &input_file->skins[skin_index];
      input_joint_count += skin->joints_count;
    }
  }
  // Determine which input joints are unecessary as no vertices reference them or any of their children
  bool* input_joint_necessary;
  {
    input_joint_necessary = malloc(sizeof(bool) * input_joint_count);
    assert(input_joint_necessary);

    bool* input_joint_referenced;
    input_joint_referenced = malloc(sizeof(bool) * input_joint_count);
    assert(input_joint_referenced);

    // Mark the input joints that are never referenced by any vertex
    for (cgltf_size input_joint_index = 0; input_joint_index < input_joint_count; ++input_joint_index)
    {
      input_joint_referenced[input_joint_index] = is_joint_referenced(input_file, input_joint_index);
    }

    // Then find the input joints that are unreferenced and only have unreferenced children and count the necessary
    // output joints
    cgltf_size input_joint_index = 0;
    for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
    {
      const cgltf_skin* skin = &input_file->skins[skin_index];
      for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
      {
        const cgltf_node* input_joint = skin->joints[joint_index];

        input_joint_necessary[input_joint_index] =
          !is_input_joint_unnecessary(input_file, skin, input_joint, input_joint_referenced);

        if (input_joint_necessary[input_joint_index])
        {
          ++output_joint_count;
        }

        ++input_joint_index;
      }
    }

    free(input_joint_referenced);
  }

  // Allocate the output joints
  {
    const cgltf_size output_joints_size = sizeof(OutputJoint) * output_joint_count;
    output_joints = malloc(output_joints_size);
    assert(output_joints);
    memset(output_joints, 0, output_joints_size);
  }

  // Populate the output joints with their respective input joint and inverse bind matrix
  cgltf_size input_joint_index = 0, output_joint_index = 0;
  for (cgltf_size skin_index = 0; skin_index < input_file->skins_count; ++skin_index)
  {
    cgltf_skin* skin = &input_file->skins[skin_index];
    for (cgltf_size joint_index = 0; joint_index < skin->joints_count; ++joint_index)
    {
      if (!input_joint_necessary[input_joint_index])
      {
        ++input_joint_index;
        continue;
      }

      OutputJoint* output_joint = &output_joints[output_joint_index];

      // Input joint
      output_joint->input_joint = skin->joints[joint_index];

      // Calculate the inverse of the global node transform of the root node in the skin
      mat4 inv_root_global_transform;
      {
        calculate_global_node_transform(skin->skeleton, inv_root_global_transform);
        glm_mat4_inv(inv_root_global_transform, inv_root_global_transform);
      }

      // Inverse bind matrix
      {
        mat4 inv_bind_matrix; // Transforms from joint to skin space
        cgltf_accessor_read_float(skin->inverse_bind_matrices, joint_index, (cgltf_float*)inv_bind_matrix, 16);

        // Transform from joint to model space by adding the transform from skin to model space (the inverse of the
        // root's global transform) to the transform from joint to skin space
        glm_mat4_mul(inv_bind_matrix, inv_root_global_transform, output_joint->inverse_bind_matrix);
      }

      ++input_joint_index;
      ++output_joint_index;
    }
  }

  // Then populate the parent joint index
  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    OutputJoint* joint = &output_joints[output_joint_index];

    const cgltf_node* parent_joint = joint->input_joint->parent;
    joint->parent_index = calculate_output_joint_index(parent_joint);

    assert(joint->parent_index < (int32_t)output_joint_count);
  }
}

uint32_t get_joint_count()
{
  return (uint32_t)output_joint_count;
}

void write_joints(FILE* output_file)
{
  for (cgltf_size output_joint_index = 0; output_joint_index < output_joint_count; ++output_joint_index)
  {
    OutputJoint* joint = &output_joints[output_joint_index];

    char name[AEM_STRING_SIZE];
    {
      sprintf(name, "%s", joint->input_joint->name); // Null-terminates string
      fwrite(name, AEM_STRING_SIZE, 1, output_file);
    }

    fwrite(&joint->inverse_bind_matrix, sizeof(joint->inverse_bind_matrix), 1, output_file);
    fwrite(&joint->parent_index, sizeof(joint->parent_index), 1, output_file);

#ifdef PRINT_JOINTS
    printf("Joint #%llu \"%s\":\n", output_joint_index, joint->input_joint->name);

    printf("\tParent index: %d\n", joint->parent_index);
    printf("\tInverse bind matrix: ");
    glm_mat4_print(joint->inverse_bind_matrix, stdout);
#endif

    int32_t padding = 0;
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);
    fwrite(&padding, sizeof(padding), 1, output_file);
  }
}

void destroy_joint_output()
{
  if (output_joints)
  {
    free(output_joints);
  }
}