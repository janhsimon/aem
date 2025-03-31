#include "geometry.h"

#include "config.h"
#include "joint.h"
#include "transform.h"

#include <cglm/affine.h>
#include <cglm/ivec4.h>
#include <cglm/mat3.h>
#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>
#include <cgltf/cgltf.h>

#include <aem/aem.h>

#include <assert.h>

typedef struct
{
  cgltf_mesh* input_mesh;

  vec3 *positions, *normals, *tangents, *bitangents;
  vec2* uvs;
  ivec4* joints;
  vec4* weights;

  uint32_t* indices;

  cgltf_size vertex_count, index_count, first_index;
  int32_t material_index;
} OutputMesh;

static cgltf_size output_mesh_count;
static OutputMesh* output_meshes = NULL;

static void locate_attributes_for_primitive(const cgltf_primitive* primitive,
                                            cgltf_attribute** positions,
                                            cgltf_attribute** normals,
                                            cgltf_attribute** tangents,
                                            cgltf_attribute** uvs,
                                            cgltf_attribute** joints,
                                            cgltf_attribute** weights)
{
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
      if (uvs)
      {
        *uvs = attribute;
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

static cgltf_node* find_node_for_mesh(const cgltf_data* input_file, const cgltf_mesh* mesh)
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

static void reconstruct_tangents(const cgltf_data* input_file,
                                 OutputMesh* output_mesh,
                                 cgltf_accessor* positions,
                                 cgltf_accessor* normals,
                                 cgltf_accessor* uvs,
                                 cgltf_accessor* indices,
                                 vec4* out)
{
  vec3* helper = malloc(sizeof(vec3) * output_mesh->vertex_count * 2);
  assert(helper);

  // Method from https://terathon.com/blog/tangent-space.html
  for (cgltf_size triangle = 0; triangle < output_mesh->index_count; triangle += 3)
  {
    // Retrieve the indices that make up this triangle
    const cgltf_size index0 = cgltf_accessor_read_index(indices, triangle + 0);
    const cgltf_size index1 = cgltf_accessor_read_index(indices, triangle + 1);
    const cgltf_size index2 = cgltf_accessor_read_index(indices, triangle + 2);

    // Retrieve the positions for the vertices of this triangle
    vec3 position0, position1, position2;
    {
      cgltf_bool result = cgltf_accessor_read_float(positions, index0, position0, 3);
      assert(result);

      result = cgltf_accessor_read_float(positions, index1, position1, 3);
      assert(result);

      result = cgltf_accessor_read_float(positions, index2, position2, 3);
      assert(result);
    }

    // Retrieve the uvs for the vertices of this triangle
    vec2 uv0, uv1, uv2;
    {
      cgltf_bool result = cgltf_accessor_read_float(uvs, index0, uv0, 2);
      assert(result);

      result = cgltf_accessor_read_float(uvs, index1, uv1, 2);
      assert(result);

      result = cgltf_accessor_read_float(uvs, index2, uv2, 2);
      assert(result);
    }

    const float x1 = position1[0] - position0[0];
    const float x2 = position2[0] - position0[0];
    const float y1 = position1[1] - position0[1];
    const float y2 = position2[1] - position0[1];
    const float z1 = position1[2] - position0[2];
    const float z2 = position2[2] - position0[2];

    const float s1 = uv1[0] - uv0[0];
    const float s2 = uv2[0] - uv0[0];
    const float t1 = uv1[1] - uv0[1];
    const float t2 = uv2[1] - uv0[1];

    const float r = 1.0f / (s1 * t2 - s2 * t1);

    vec3 sdir;
    sdir[0] = (t2 * x1 - t1 * x2) * r;
    sdir[1] = (t2 * y1 - t1 * y2) * r;
    sdir[2] = (t2 * z1 - t1 * z2) * r;

    vec3 tdir;
    tdir[0] = (s1 * x2 - s2 * x1) * r;
    tdir[1] = (s1 * y2 - s2 * y1) * r;
    tdir[2] = (s1 * z2 - s2 * z1) * r;

    glm_vec3_add(helper[index0], sdir, helper[index0]);
    glm_vec3_add(helper[index1], sdir, helper[index1]);
    glm_vec3_add(helper[index2], sdir, helper[index2]);

    glm_vec3_add(helper[index0 + output_mesh->vertex_count], tdir, helper[index0 + output_mesh->vertex_count]);
    glm_vec3_add(helper[index1 + output_mesh->vertex_count], tdir, helper[index1 + output_mesh->vertex_count]);
    glm_vec3_add(helper[index2 + output_mesh->vertex_count], tdir, helper[index2 + output_mesh->vertex_count]);
  }

  for (cgltf_size vertex_index = 0; vertex_index < output_mesh->vertex_count; ++vertex_index)
  {
    vec3 normal;
    {
      const cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, normal, 3);
      assert(result);
    }

    // Gram-Schmidt orthogonalize
    {
      const float dot = glm_dot(normal, helper[vertex_index]);

      vec3 ndot;
      glm_vec3_scale(normal, dot, ndot);

      glm_vec3_sub(helper[vertex_index], ndot, out[vertex_index]);
      glm_vec3_normalize(out[vertex_index]);
    }

    // Calculate handedness
    {
      vec3 cross;
      glm_vec3_cross(normal, helper[vertex_index], cross);

      const float dot = glm_dot(cross, helper[vertex_index + output_mesh->vertex_count]);

      out[vertex_index][3] = (dot < 0.0f) ? -1.0f : 1.0f;
    }
  }

  free(helper);
}

static bool check_node_for_instancing(const cgltf_data* input_file, const cgltf_node* node, bool* mesh_referenced)
{
  const cgltf_mesh* mesh = node->mesh;
  if (mesh)
  {
    for (cgltf_size mesh_index = 0; mesh_index < input_file->meshes_count; ++mesh_index)
    {
      if (mesh == &input_file->meshes[mesh_index])
      {
        if (mesh_referenced[mesh_index])
        {
          return true;
        }

        mesh_referenced[mesh_index] = true;
        break;
      }
    }
  }

  for (cgltf_size child_index = 0; child_index < node->children_count; ++child_index)
  {
    const cgltf_node* child = node->children[child_index];
    if (check_node_for_instancing(input_file, child, mesh_referenced))
    {
      return true;
    }
  }

  return false;
}

static bool
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

static void add_vertices_to_output_mesh(OutputMesh* output_mesh,
                                        cgltf_data* input_file,
                                        cgltf_skin* skin,
                                        cgltf_attribute* positions,
                                        cgltf_attribute* normals,
                                        cgltf_attribute* tangents,
                                        cgltf_attribute* uvs,
                                        cgltf_attribute* joints,
                                        cgltf_attribute* weights,
                                        cgltf_accessor* indices,
                                        mat4 global_node_transform)
{
  mat3 global_node_rotation;
  glm_mat4_pick3(global_node_transform, global_node_rotation);

  glm_vec3_normalize(global_node_rotation[0]);
  glm_vec3_normalize(global_node_rotation[1]);
  glm_vec3_normalize(global_node_rotation[2]);

  // Reconstruct tangents if they are not included and uvs exist
  vec4* reconstructed_tangents = NULL;
  if (!tangents && uvs)
  {
    reconstructed_tangents = malloc(sizeof(vec4) * output_mesh->vertex_count);
    assert(reconstructed_tangents);

    reconstruct_tangents(input_file, output_mesh, positions->data, normals->data, uvs->data, indices,
                         reconstructed_tangents);
  }

  for (cgltf_size vertex_index = 0; vertex_index < output_mesh->vertex_count; ++vertex_index)
  {
    // Position (required)
    {
      const cgltf_bool result =
        cgltf_accessor_read_float(positions->data, vertex_index, output_mesh->positions[vertex_index], 3);
      assert(result);

      glm_mat4_mulv3(global_node_transform, output_mesh->positions[vertex_index], 1.0f,
                     output_mesh->positions[vertex_index]);
    }

    // Normal (required)
    {
      const cgltf_bool result =
        cgltf_accessor_read_float(normals->data, vertex_index, output_mesh->normals[vertex_index], 3);
      assert(result);

      glm_mat3_mulv(global_node_rotation, output_mesh->normals[vertex_index], output_mesh->normals[vertex_index]);
    }

    // Tangent (included or reconstructed)
    vec4 tangent; // With extra w-channel for later bitangent construction
    {
      if (tangents)
      {
        const cgltf_bool result = cgltf_accessor_read_float(tangents->data, vertex_index, tangent, 4);
        assert(result);
      }
      else if (reconstructed_tangents)
      {
        glm_vec4_copy(reconstructed_tangents[vertex_index], tangent);
      }

      glm_vec3_copy(tangent, output_mesh->tangents[vertex_index]);
      glm_mat3_mulv(global_node_rotation, output_mesh->tangents[vertex_index], output_mesh->tangents[vertex_index]);
    }

    // Bitangent (constructed)
    {
      glm_cross(output_mesh->normals[vertex_index], output_mesh->tangents[vertex_index],
                output_mesh->bitangents[vertex_index]);
      glm_vec3_scale(output_mesh->bitangents[vertex_index], tangent[3], output_mesh->bitangents[vertex_index]);
    }

    // UV (optional)
    if (uvs)
    {
      const cgltf_bool result = cgltf_accessor_read_float(uvs->data, vertex_index, output_mesh->uvs[vertex_index], 2);
      assert(result);
    }
    else
    {
      glm_vec2_zero(output_mesh->uvs[vertex_index]);
    }

    // Joints (optional)
    if (joints)
    {
      assert(skin);

      const cgltf_bool result =
        cgltf_accessor_read_uint(joints->data, vertex_index, output_mesh->joints[vertex_index], 4);
      assert(result);

      // Convert GLB to AEM joint indices
      for (cgltf_size i = 0; i < 4; ++i)
      {
        const cgltf_size glb_joint_index = output_mesh->joints[vertex_index][i];
        const cgltf_node* glb_joint = skin->joints[glb_joint_index];
        output_mesh->joints[vertex_index][i] = calculate_aem_joint_index_from_glb_joint(glb_joint);
      }
    }
    else
    {
      output_mesh->joints[vertex_index][0] = output_mesh->joints[vertex_index][1] =
        output_mesh->joints[vertex_index][2] = output_mesh->joints[vertex_index][3] = -1;
    }

    // Weights (optional)
    if (weights)
    {
      const cgltf_bool result =
        cgltf_accessor_read_float(weights->data, vertex_index, output_mesh->weights[vertex_index], 4);
      assert(result);
    }
    else
    {
      glm_vec4_zero(output_mesh->weights[vertex_index]);
    }
  }

  if (reconstructed_tangents)
  {
    free(reconstructed_tangents);
  }
}

uint32_t get_mesh_count()
{
  return (uint32_t)output_mesh_count;
}

void setup_geometry_output(const cgltf_data* input_file)
{
  // Check that there are never two primitives that have triangles
  for (cgltf_size mesh_index = 0; mesh_index < input_file->meshes_count; ++mesh_index)
  {
    const cgltf_mesh* mesh = &input_file->meshes[mesh_index];

    bool has_triangles = false;
    for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
      const cgltf_primitive* primitive = &mesh->primitives[primitive_index];
      if (primitive->type == cgltf_primitive_type_triangles)
      {
        assert(!has_triangles);
        has_triangles = true;
      }
    }
  }

  // Count the required output meshes
  output_mesh_count = 0;
  for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
  {
    const cgltf_mesh* mesh = input_file->nodes[node_index].mesh;
    if (!mesh)
    {
      continue;
    }

    for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
      const cgltf_primitive* primitive = &mesh->primitives[primitive_index];

      cgltf_attribute *positions = NULL, *normals = NULL;
      locate_attributes_for_primitive(primitive, &positions, &normals, NULL, NULL, NULL, NULL);

      if (is_primitive_valid(primitive, positions, normals))
      {
        ++output_mesh_count;
        break;
      }
    }
  }

  assert(output_mesh_count > 0);

  // Allocate the output meshes
  {
    const cgltf_size output_meshes_size = sizeof(OutputMesh) * output_mesh_count;
    output_meshes = malloc(output_meshes_size);
    assert(output_meshes);
    memset(output_meshes, 0, output_meshes_size);
  }

  // Count the mesh vertices and indices and allocate space for them
  {
    cgltf_size output_mesh_index = 0;
    cgltf_size first_vertex = 0, first_index = 0;
    for (cgltf_size node_index = 0; node_index < input_file->nodes_count; ++node_index)
    {
      const cgltf_node* node = &input_file->nodes[node_index];
      const cgltf_mesh* mesh = node->mesh;
      if (!mesh)
      {
        continue;
      }

      const cgltf_skin* skin = node->skin;

      OutputMesh* output_mesh = &output_meshes[output_mesh_index];
      output_mesh->vertex_count = output_mesh->index_count = 0;
      for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
      {
        const cgltf_primitive* primitive = &mesh->primitives[primitive_index];

        cgltf_attribute *positions = NULL, *normals = NULL, *tangents = NULL, *uvs = NULL, *joints = NULL,
                        *weights = NULL;
        locate_attributes_for_primitive(primitive, &positions, &normals, &tangents, &uvs, &joints, &weights);

        if (!is_primitive_valid(primitive, positions, normals))
        {
          continue;
        }

        output_mesh->vertex_count += positions->data->count;
        output_mesh->index_count += primitive->indices->count;

        assert(normals->data->count == output_mesh->vertex_count);

        if (tangents)
        {
          assert(tangents->data->count == output_mesh->vertex_count);
        }

        if (uvs)
        {
          assert(uvs->data->count == output_mesh->vertex_count);
        }

        if (joints || weights)
        {
          assert(joints && weights);
        }
      }

      // Allocate vertices
      output_mesh->positions = malloc(sizeof(*output_mesh->positions) * output_mesh->vertex_count);
      output_mesh->normals = malloc(sizeof(*output_mesh->normals) * output_mesh->vertex_count);
      output_mesh->tangents = malloc(sizeof(*output_mesh->tangents) * output_mesh->vertex_count);
      output_mesh->bitangents = malloc(sizeof(*output_mesh->bitangents) * output_mesh->vertex_count);
      output_mesh->uvs = malloc(sizeof(*output_mesh->uvs) * output_mesh->vertex_count);
      output_mesh->joints = malloc(sizeof(*output_mesh->joints) * output_mesh->vertex_count);
      output_mesh->weights = malloc(sizeof(*output_mesh->weights) * output_mesh->vertex_count);

      // Allocate indices
      output_mesh->indices = malloc(sizeof(*output_mesh->indices) * output_mesh->index_count);
      assert(output_mesh->indices);

      mat4 global_node_transform;
      calculate_global_node_transform(node, global_node_transform);

      // Fill in the vertices and indices
      for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
      {
        const cgltf_primitive* primitive = &mesh->primitives[primitive_index];

        cgltf_attribute *positions = NULL, *normals = NULL, *tangents = NULL, *uvs = NULL, *joints = NULL,
                        *weights = NULL;
        locate_attributes_for_primitive(primitive, &positions, &normals, &tangents, &uvs, &joints, &weights);

        if (!is_primitive_valid(primitive, positions, normals))
        {
          continue;
        }

        add_vertices_to_output_mesh(output_mesh, input_file, skin, positions, normals, tangents, uvs, joints, weights,
                                    primitive->indices, global_node_transform);

        for (cgltf_size index = 0; index < output_mesh->index_count; ++index)
        {
          output_mesh->indices[index] = cgltf_accessor_read_index(primitive->indices, index) + first_vertex;
        }

        output_mesh->first_index = first_index;

        output_mesh->input_mesh = mesh;

        // Material index
        if (primitive->material)
        {
          output_mesh->material_index = cgltf_material_index(input_file, primitive->material);
        }
        else
        {
          output_mesh->material_index = -1;
        }
      }

      ++output_mesh_index;
      first_vertex += output_mesh->vertex_count;
      first_index += output_mesh->index_count;
    }
  }
}

uint64_t calculate_vertex_buffer_size()
{
  cgltf_size vertex_buffer_size = 0;
  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];
    vertex_buffer_size += output_mesh->vertex_count;
  }

  return (uint64_t)(vertex_buffer_size * AEM_VERTEX_SIZE);
}

uint64_t calculate_index_buffer_size()
{
  cgltf_size index_buffer_size = 0;
  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];
    index_buffer_size += output_mesh->index_count;
  }

  return (uint64_t)(index_buffer_size * AEM_INDEX_SIZE);
}

void write_vertex_buffer(FILE* output_file)
{
  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];

    for (cgltf_size vertex_index = 0; vertex_index < output_mesh->vertex_count; ++vertex_index)
    {
      fwrite(output_mesh->positions[vertex_index], sizeof(output_mesh->positions[vertex_index]), 1, output_file);
      fwrite(output_mesh->normals[vertex_index], sizeof(output_mesh->normals[vertex_index]), 1, output_file);
      fwrite(output_mesh->tangents[vertex_index], sizeof(output_mesh->tangents[vertex_index]), 1, output_file);
      fwrite(output_mesh->bitangents[vertex_index], sizeof(output_mesh->bitangents[vertex_index]), 1, output_file);
      fwrite(output_mesh->uvs[vertex_index], sizeof(output_mesh->uvs[vertex_index]), 1, output_file);
      fwrite(output_mesh->joints[vertex_index], sizeof(output_mesh->joints[vertex_index]), 1, output_file);
      fwrite(output_mesh->weights[vertex_index], sizeof(output_mesh->weights[vertex_index]), 1, output_file);

      // Extra joint index
      {
        const int32_t minus_one = -1;
        fwrite(&minus_one, sizeof(minus_one), 1, output_file);
      }

#ifdef PRINT_VERTEX_BUFFER
      static uint64_t vertex_counter = 0;
      printf("Vertex #%llu:\n", vertex_counter++);

      printf("\tPosition: [ %f, %f, %f ]\n", output_mesh->positions[vertex_index][0],
             output_mesh->positions[vertex_index][1], output_mesh->positions[vertex_index][2]);

      printf("\tNormal: [ %f, %f, %f ]\n", output_mesh->normals[vertex_index][0], output_mesh->normals[vertex_index][1],
             output_mesh->normals[vertex_index][2]);

      printf("\tTangent: [ %f, %f, %f ]\n", output_mesh->tangents[vertex_index][0],
             output_mesh->tangents[vertex_index][1], output_mesh->tangents[vertex_index][2]);

      printf("\tBitangent: [ %f, %f, %f ]\n", output_mesh->bitangents[vertex_index][0],
             output_mesh->bitangents[vertex_index][1], output_mesh->bitangents[vertex_index][2]);

      printf("\tUV: [ %f, %f ]\n", output_mesh->uvs[vertex_index][0], output_mesh->uvs[vertex_index][1]);

      printf("\tJoints: [ %d, %d, %d, %d ]\n", output_mesh->joints[vertex_index][0],
             output_mesh->joints[vertex_index][1], output_mesh->joints[vertex_index][2],
             output_mesh->joints[vertex_index][3]);

      printf("\tWeights: [ %f, %f, %f, %f ]\n", output_mesh->weights[vertex_index][0],
             output_mesh->weights[vertex_index][1], output_mesh->weights[vertex_index][2],
             output_mesh->weights[vertex_index][3]);

#endif
    }
  }
}

void write_index_buffer(FILE* output_file)
{
  /*cgltf_size index_counter = 0;
  cgltf_size index_offset = 0;
  for (cgltf_size mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct MeshInfo* mesh_info = &mesh_infos[mesh_index];

    for (cgltf_size index = 0; index < mesh_info->indices->count; ++index)
    {
      const uint32_t value = (uint32_t)(cgltf_accessor_read_index(mesh_info->indices, index) + index_offset);
      fwrite(&value, sizeof(value), 1, output_file);

#ifdef PRINT_INDEX_BUFFER
      printf("Index #%llu: %u\n", index_counter++, value);
#endif
    }

    index_offset += mesh_info->vertex_count;
  }*/

  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];
    fwrite(output_mesh->indices, output_mesh->index_count * sizeof(*output_mesh->indices), 1, output_file);
  }
}

void write_meshes(FILE* output_file)
{
  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];

    const uint32_t first_index = (uint32_t)output_mesh->first_index;
    fwrite(&first_index, sizeof(first_index), 1, output_file);

    const uint32_t index_count = (uint32_t)output_mesh->index_count;
    fwrite(&index_count, sizeof(index_count), 1, output_file);

    const int32_t material_index = output_mesh->material_index;
    fwrite(&material_index, sizeof(material_index), 1, output_file);

#ifdef PRINT_MESHES
    printf("Mesh #%llu \"%s\":\n", mesh_index, output_mesh->input_mesh->name);
    printf("\tFirst index: %u\n", first_index);
    printf("\tIndex count: %u\n", index_count);
    printf("\tMaterial index: %d\n", material_index);
#endif
  }

  /* uint32_t index_offset = 0;
   for (cgltf_size mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
   {
     const struct MeshInfo* mesh_info = &mesh_infos[mesh_index];

     fwrite(&index_offset, sizeof(index_offset), 1, output_file);

     const uint32_t index_count = (uint32_t)mesh_info->indices->count;
     fwrite(&index_count, sizeof(index_count), 1, output_file);

     const uint32_t material_index = mesh_info->material_index;
     fwrite(&material_index, sizeof(material_index), 1, output_file);

 #ifdef PRINT_MESHES
     printf("Mesh #%llu:\n", mesh_index);
     printf("\tFirst index: %u\n", index_offset);
     printf("\tIndex count: %u\n", index_count);
     printf("\tMaterial index: %u\n", material_index);
 #endif

     index_offset += index_count;
   }*/
}

void destroy_geometry_output()
{
  for (cgltf_size mesh_index = 0; mesh_index < output_mesh_count; ++mesh_index)
  {
    const OutputMesh* output_mesh = &output_meshes[mesh_index];

    free(output_mesh->positions);
    free(output_mesh->normals);
    free(output_mesh->tangents);
    free(output_mesh->bitangents);
    free(output_mesh->uvs);

    free(output_mesh->indices);
  }

  free(output_meshes);

  /*for (cgltf_size mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
  {
    const struct MeshInfo* mesh_info = &mesh_infos[mesh_index];
    if (mesh_info->reconstructed_tangents)
    {
      free(mesh_info->reconstructed_tangents);
    }
  }

  free(mesh_infos);*/
}