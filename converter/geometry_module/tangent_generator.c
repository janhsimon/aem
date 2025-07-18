#include "tangent_generator.h"

#include "output_mesh.h"

#include <cgltf/cgltf.h>

#include <cglm/vec3.h>

#include <assert.h>
#include <string.h>

static void generate_tangent_without_uvs(vec3 n, vec3 tangent, vec3 bitangent)
{
  // Choose the smallest component to avoid numerical instability
  if (fabsf(n[0]) < fabsf(n[1]) && fabsf(n[0]) < fabsf(n[2]))
  {
    glm_vec3_cross((vec3){ 1, 0, 0 }, n, tangent);
  }
  else if (fabsf(n[1]) < fabsf(n[2]))
  {
    glm_vec3_cross((vec3){ 0, 1, 0 }, n, tangent);
  }
  else
  {
    glm_vec3_cross((vec3){ 0, 0, 1 }, n, tangent);
  }

  glm_vec3_normalize(tangent);      // Tangent
  glm_cross(n, tangent, bitangent); // Bitangent
}

void generate_tangents_with_uvs(const OutputMesh* output_mesh,
                                const cgltf_accessor* positions,
                                const cgltf_accessor* normals,
                                const cgltf_accessor* uvs,
                                const cgltf_accessor* indices)
{
  const size_t vertex_count = output_mesh->vertex_count;
  const size_t index_count = output_mesh->index_count;

  typedef struct
  {
    vec3 tan1, tan2;
    bool valid;
  } Helper;

  Helper* helpers;
  {
    const size_t size = sizeof(Helper) * vertex_count;
    helpers = malloc(size);
    assert(helpers);
    memset(helpers, 0, size);
  }

  for (cgltf_size index = 0; index < index_count; index += 3)
  {
    const cgltf_size i0 = cgltf_accessor_read_index(indices, index + 0);
    const cgltf_size i1 = cgltf_accessor_read_index(indices, index + 1);
    const cgltf_size i2 = cgltf_accessor_read_index(indices, index + 2);

    vec3 p0, p1, p2;
    vec2 uv0, uv1, uv2;

    cgltf_accessor_read_float(positions, i0, p0, 3);
    cgltf_accessor_read_float(positions, i1, p1, 3);
    cgltf_accessor_read_float(positions, i2, p2, 3);

    cgltf_accessor_read_float(uvs, i0, uv0, 2);
    cgltf_accessor_read_float(uvs, i1, uv1, 2);
    cgltf_accessor_read_float(uvs, i2, uv2, 2);

    const float x1 = p1[0] - p0[0];
    const float x2 = p2[0] - p0[0];
    const float y1 = p1[1] - p0[1];
    const float y2 = p2[1] - p0[1];
    const float z1 = p1[2] - p0[2];
    const float z2 = p2[2] - p0[2];

    const float s1 = uv1[0] - uv0[0];
    const float s2 = uv2[0] - uv0[0];
    const float t1 = uv1[1] - uv0[1];
    const float t2 = uv2[1] - uv0[1];

    const float denom = s1 * t2 - s2 * t1;
    if (fabsf(denom) < 1e-8f)
    {
      continue;
    }

    const float r = 1.0f / denom;

    vec3 sdir = {
      (t2 * x1 - t1 * x2) * r,
      (t2 * y1 - t1 * y2) * r,
      (t2 * z1 - t1 * z2) * r,
    };

    vec3 tdir = {
      (s1 * x2 - s2 * x1) * r,
      (s1 * y2 - s2 * y1) * r,
      (s1 * z2 - s2 * z1) * r,
    };

    Helper* helper0 = &helpers[i0];
    Helper* helper1 = &helpers[i1];
    Helper* helper2 = &helpers[i2];

    glm_vec3_add(helper0->tan1, sdir, helper0->tan1);
    glm_vec3_add(helper1->tan1, sdir, helper1->tan1);
    glm_vec3_add(helper2->tan1, sdir, helper2->tan1);

    glm_vec3_add(helper0->tan2, tdir, helper0->tan2);
    glm_vec3_add(helper1->tan2, tdir, helper1->tan2);
    glm_vec3_add(helper2->tan2, tdir, helper2->tan2);

    helper0->valid = helper1->valid = helper2->valid = true;
  }

  for (cgltf_size vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
  {
    Helper* helper = &helpers[vertex_index];

    vec3 normal;
    cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, normal, 3);
    assert(result);
    glm_vec3_normalize(normal);

    if (!helper->valid)
    {
      generate_tangent_without_uvs(normal, output_mesh->tangents[vertex_index], output_mesh->bitangents[vertex_index]);
    }
    else
    {
      vec3 temp;
      glm_vec3_copy(helper->tan1, temp); // Copy before modification
      glm_normalize(temp);

      // Gram-Schmidt orthogonalization
      vec3 n_dot_t;
      glm_vec3_scale(normal, glm_dot(normal, helper->tan1), n_dot_t);
      glm_vec3_sub(helper->tan1, n_dot_t, helper->tan1);
      glm_vec3_normalize(helper->tan1);

      // Store the tangent xyz
      glm_vec3_copy(helper->tan1, output_mesh->tangents[vertex_index]);

      // Calculate handedness (sign)
      vec3 cross;
      glm_vec3_cross(normal, helper->tan1, cross);
      const float handedness = (glm_dot(cross, helper->tan2) < 0.0f) ? -1.0f : 1.0f;

      // Bitangent
      glm_cross(normal, temp, output_mesh->bitangents[vertex_index]);
      glm_vec3_scale(output_mesh->bitangents[vertex_index], handedness, output_mesh->bitangents[vertex_index]);
    }
  }

  free(helpers);
}

void generate_tangents_without_uvs(const OutputMesh* output_mesh, const cgltf_accessor* normals)
{
  const size_t vertex_count = output_mesh->vertex_count;
  for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
  {
    vec3 normal;
    cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, normal, 3);
    assert(result);
    glm_vec3_normalize(normal);

    generate_tangent_without_uvs(normal, output_mesh->tangents[vertex_index], output_mesh->bitangents[vertex_index]);
  }
}

void generate_tangents_from_normals_tangents(const OutputMesh* output_mesh,
                                             const cgltf_accessor* normals,
                                             const cgltf_accessor* tangents)
{
  const size_t vertex_count = output_mesh->vertex_count;
  for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
  {
    vec3 normal;
    cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, normal, 3);
    assert(result);
    glm_vec3_normalize(normal);

    vec4 tangent;
    result = cgltf_accessor_read_float(tangents, vertex_index, tangent, 4); // Fourth component is handedness
    assert(result);

    // Tangent
    glm_vec3_copy(tangent, output_mesh->tangents[vertex_index]);
    glm_vec3_normalize(output_mesh->tangents[vertex_index]);

    // Bitangent
    glm_cross(normal, output_mesh->tangents[vertex_index], output_mesh->bitangents[vertex_index]);
    glm_vec3_scale(output_mesh->bitangents[vertex_index], tangent[3], output_mesh->bitangents[vertex_index]);
  }
}