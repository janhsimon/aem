#include "material_module.h"

#include <cgltf/cgltf.h>

#include <cglm/affine2d.h>
#include <cglm/vec2.h>

void make_identity_transform(cgltf_texture_transform* transform)
{
  transform->offset[0] = transform->offset[1] = 0.0f;
  transform->rotation = 0.0f;
  transform->scale[0] = transform->scale[1] = 1.0f;
}

void copy_transforms(const cgltf_texture_transform* from, cgltf_texture_transform* to)
{
  to->offset[0] = from->offset[0];
  to->offset[1] = from->offset[1];

  to->rotation = from->rotation;

  to->scale[0] = from->scale[0];
  to->scale[1] = from->scale[1];
}

cgltf_texture_transform make_transform_for_texture_view(const cgltf_texture_view* texture_view)
{
  cgltf_texture_transform transform;
  if (texture_view->has_transform)
  {
    copy_transforms(&texture_view->transform, &transform);
  }
  else
  {
    make_identity_transform(&transform);
  }

  return transform;
}

bool compare_transforms(cgltf_texture_transform* a, cgltf_texture_transform* b)
{
  if (!glm_vec2_eqv_eps(a->offset, b->offset))
  {
    return false;
  }

  if (fabsf(a->rotation - b->rotation) > GLM_FLT_EPSILON)
  {
    return false;
  }

  if (!glm_vec2_eqv_eps(a->scale, b->scale))
  {
    return false;
  }

  return true;
}

bool is_transform_identity(cgltf_texture_transform* transform)
{
  if (!glm_vec2_eqv_eps(transform->offset, GLM_VEC2_ZERO))
  {
    return false;
  }

  if (fabsf(transform->rotation) > GLM_FLT_EPSILON)
  {
    return false;
  }

  if (!glm_vec2_eqv_eps(transform->scale, GLM_VEC2_ONE))
  {
    return false;
  }

  return true;
}

void transform_to_mat3(cgltf_texture_transform* transform, mat3 matrix)
{
  glm_translate2d_make(matrix, transform->offset);
  glm_rotate2d(matrix, transform->rotation);
  glm_scale2d(matrix, transform->scale);
}