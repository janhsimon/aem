#include "directional_light.h"

#include "preferences.h"

#include <cglm/cam.h>
#include <cglm/frustum.h>
#include <cglm/mat4.h>
#include <cglm/vec3.h>

static mat4 view_matrix, proj_matrix, viewproj_matrix;

void directional_light_calc_viewproj(struct Preferences* preferences,
                                     vec4 frustum_corners[8],
                                     vec4 frustum_center)
{
  // View matrix
  {
    vec3 step;
    glm_vec3_sub(frustum_corners[4], frustum_center, step);
    const float step_length = glm_vec3_norm(step);
    glm_vec3_scale_as(preferences->light_dir, -step_length, step);

    vec3 eye;
    glm_vec3_add(frustum_center, step, eye);

    glm_lookat(eye, frustum_center, GLM_YUP, view_matrix);
  }

  // Projection matrix
  {
    vec3 box[2];
    glm_frustum_box(frustum_corners, view_matrix, box);
    glm_ortho_aabb_pz(box, 25.0f, proj_matrix);
  }

  glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);
}

void directional_light_get_view_matrix(mat4 view)
{
  glm_mat4_copy(view_matrix, view);
}

void directional_light_get_proj_matrix(mat4 proj)
{
  glm_mat4_copy(proj_matrix, proj);
}

void directional_light_get_viewproj_matrix(mat4 viewproj)
{
  glm_mat4_copy(viewproj_matrix, viewproj);
}
