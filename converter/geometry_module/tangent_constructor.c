#include "tangent_constructor.h"

#include "output_mesh.h"

#include <cgltf/cgltf.h>

#include <cglm/vec3.h>

#include <assert.h>

void reconstruct_tangents(const cgltf_data* input_file,
                          OutputMesh* output_mesh,
                          cgltf_accessor* positions,
                          cgltf_accessor* normals,
                          cgltf_accessor* uvs,
                          cgltf_accessor* indices,
                          vec4* original_tangents,
                          vec4* reconstructed_tangents)
{
  const size_t helper_size = sizeof(vec3) * output_mesh->vertex_count * 2;
  vec3* helper = malloc(helper_size);
  assert(helper);
  memset(helper, 0, helper_size);

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

    const float denom = s1 * t2 - s2 * t1;
    if (fabsf(denom) < 1e-8f)
    {
      continue; // Skip degenerate triangle
    }
    const float r = 1.0f / denom;

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
    glm_vec3_copy(helper[vertex_index], original_tangents[vertex_index]); // Copy before modification
    glm_normalize(original_tangents[vertex_index]);

    vec3 normal;
    {
      const cgltf_bool result = cgltf_accessor_read_float(normals, vertex_index, normal, 3);
      assert(result);

      glm_vec3_normalize(normal);
    }

    // Gram-Schmidt orthogonalize
    {
      const float dot = glm_dot(normal, helper[vertex_index]);

      vec3 ndot;
      glm_vec3_scale(normal, dot, ndot);

      glm_vec3_sub(helper[vertex_index], ndot, reconstructed_tangents[vertex_index]);
      glm_vec3_normalize(reconstructed_tangents[vertex_index]);
    }

    // Calculate handedness
    {
      vec3 cross;
      glm_vec3_cross(normal, original_tangents[vertex_index], cross);

      const float dot = glm_dot(cross, helper[vertex_index + output_mesh->vertex_count]);
      reconstructed_tangents[vertex_index][3] = (dot < 0.0f) ? -1.0f : 1.0f;
    }
  }

  free(helper);
}