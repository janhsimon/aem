#include "tangent_generator.h"

#include "output_mesh.h"

#include <cgltf/cgltf.h>

#include <cglm/vec3.h>

#include <assert.h>
#include <string.h>

static void generate_tangent_without_uvs(vec3 n, vec4 tangent)
{
  vec3 tangent3;

  // Choose the smallest component to avoid numerical instability
  if (fabsf(n[0]) < fabsf(n[1]) && fabsf(n[0]) < fabsf(n[2]))
  {
    glm_vec3_cross((vec3){ 1, 0, 0 }, n, tangent3);
  }
  else if (fabsf(n[1]) < fabsf(n[2]))
  {
    glm_vec3_cross((vec3){ 0, 1, 0 }, n, tangent3);
  }
  else
  {
    glm_vec3_cross((vec3){ 0, 0, 1 }, n, tangent3);
  }

  glm_vec3_normalize(tangent3);

  tangent[0] = tangent3[0];
  tangent[1] = tangent3[1];
  tangent[2] = tangent3[2];
  tangent[3] = 1.0f; // default handedness
}

void generate_tangents_with_uvs(const OutputMesh* output_mesh,
                                const cgltf_accessor* positions,
                                const cgltf_accessor* normals,
                                const cgltf_accessor* uvs,
                                const cgltf_accessor* indices,
                                vec4* tangents)
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
    vec3 n;
    cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, n, 3);
    assert(result);
    glm_vec3_normalize(n);

    Helper* helper = &helpers[vertex_index];

    if (!helper->valid)
    {
      generate_tangent_without_uvs(n, tangents[vertex_index]);
    }
    else
    {
      // Gram-Schmidt orthogonalization
      vec3 n_dot_t;
      glm_vec3_scale(n, glm_dot(n, helper->tan1), n_dot_t);
      glm_vec3_sub(helper->tan1, n_dot_t, helper->tan1);
      glm_vec3_normalize(helper->tan1);

      // Store the tangent xyz
      tangents[vertex_index][0] = helper->tan1[0];
      tangents[vertex_index][1] = helper->tan1[1];
      tangents[vertex_index][2] = helper->tan1[2];

      // Calculate handedness (sign) and store it in the w component
      vec3 cross;
      glm_vec3_cross(n, helper->tan1, cross);
      const float handedness = (glm_dot(cross, helper->tan2) < 0.0f) ? -1.0f : 1.0f;
      tangents[vertex_index][3] = handedness;
    }
  }

  free(helpers);
}

void generate_tangents_without_uvs(const OutputMesh* output_mesh, const cgltf_accessor* normals, vec4* tangents)
{
  const size_t vertex_count = output_mesh->vertex_count;

  for (size_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index)
  {
    vec3 n;
    cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, n, 3);
    assert(result);
    glm_vec3_normalize(n);

    generate_tangent_without_uvs(n, tangents[vertex_index]);
  }
}