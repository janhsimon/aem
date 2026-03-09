#include "directional_light.h"

#include "camera.h"
#include "preferences.h"

#include <cglm/cam.h>
#include <cglm/frustum.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

static mat4 view_matrix[4], proj_matrix[4], viewproj_matrix[4];

void directional_light_calc_viewproj(struct Preferences* preferences, float near, float far)
{
  camera_calc_frustum(preferences, near, far);

  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    vec4 frustum_corners[8], frustum_center;
    camera_get_frustum_cascade_corners(cascade_index, frustum_corners);
    camera_get_frustum_cascade_center(cascade_index, frustum_center);

    // View matrix
    {
      vec3 step;
      glm_vec3_sub(frustum_corners[4], frustum_center, step);
      const float step_length = glm_vec3_norm(step);
      glm_vec3_scale_as(preferences->light_dir, -step_length, step);

      vec3 eye;
      glm_vec3_add(frustum_center, step, eye);

      glm_lookat(eye, frustum_center, GLM_YUP, view_matrix[cascade_index]);
    }

    // Projection matrix
    {
      vec3 box[2];
      glm_frustum_box(frustum_corners, view_matrix[cascade_index], box);
      glm_ortho_aabb_pz(box, 25.0f, proj_matrix[cascade_index]);
    }

    glm_mat4_mul(proj_matrix[cascade_index], view_matrix[cascade_index], viewproj_matrix[cascade_index]);
  }
}

void directional_light_get_view_matrix(int cascade_index, mat4 view)
{
  glm_mat4_copy(view_matrix[cascade_index], view);
}

void directional_light_get_proj_matrix(int cascade_index, mat4 proj)
{
  glm_mat4_copy(proj_matrix[cascade_index], proj);
}

void directional_light_get_viewproj_matrix(int cascade_index, mat4 viewproj)
{
  glm_mat4_copy(viewproj_matrix[cascade_index], viewproj);
}
