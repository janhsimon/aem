#include "mesh_inspector.h"

#include <cgltf/cgltf.h>

void locate_attributes_for_primitive(const cgltf_primitive* primitive,
                                            cgltf_attribute** positions,
                                            cgltf_attribute** normals,
                                            cgltf_attribute** tangents,
                                            cgltf_attribute** uvs,
                                            cgltf_attribute** joints,
                                            cgltf_attribute** weights)
{
  cgltf_size num_uv_coords = 0;
  for (cgltf_size attribute_index = 0; attribute_index < primitive->attributes_count; ++attribute_index)
  {
    cgltf_attribute* attribute = &primitive->attributes[attribute_index];

    if (attribute->type == cgltf_attribute_type_position)
    {
      if (positions)
      {
        *positions = attribute;
      }
    }
    else if (attribute->type == cgltf_attribute_type_normal)
    {
      if (normals)
      {
        *normals = attribute;
      }
    }
    else if (attribute->type == cgltf_attribute_type_tangent)
    {
      if (tangents)
      {
        *tangents = attribute;
      }
    }
    else if (attribute->type == cgltf_attribute_type_texcoord)
    {
      if (uvs && num_uv_coords == 0)
      {
        *uvs = attribute;
        ++num_uv_coords;
      }
    }
    else if (attribute->type == cgltf_attribute_type_joints)
    {
      if (joints)
      {
        *joints = attribute;
      }
    }
    else if (attribute->type == cgltf_attribute_type_weights)
    {
      if (weights)
      {
        *weights = attribute;
      }
    }
  }
}

static cgltf_node* find_node_for_mesh_recursively(cgltf_node* node, const cgltf_mesh* mesh)
{
  if (node->mesh == mesh)
  {
    return node;
  }

  for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
  {
    struct cgltf_node* child = node->children[child_index];

    struct cgltf_node* result = find_node_for_mesh_recursively(child, mesh);
    if (result)
    {
      return result;
    }
  }

  return NULL;
}

cgltf_node* find_node_for_mesh(const cgltf_data* input_file, const cgltf_mesh* mesh)
{
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    struct cgltf_node* node = &input_file->nodes[node_index];

    cgltf_node* result = find_node_for_mesh_recursively(node, mesh);
    if (result)
    {
      return result;
    }
  }

  return NULL;
}

bool
is_primitive_valid(const cgltf_primitive* primitive, const cgltf_attribute* positions, const cgltf_attribute* normals)
{
  if (primitive->type != cgltf_primitive_type_triangles)
  {
    return false;
  }

  if (!positions || !normals)
  {
    return false;
  }

  if (!primitive->indices || primitive->indices->count < 3)
  {
    return false;
  }

  return true;
}