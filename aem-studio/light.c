#include "light.h"

#include <cglm/affine.h>

static vec3 position = { 1.0f, 1.0f, -1.0f };

void light_tumble(vec2 delta)
{
  mat4 tumble;
  glm_rotate_make(tumble, delta[0], GLM_YUP);
  glm_mat4_mulv3(tumble, position, 0.0f, position);
}

void calc_light_dir(vec3 dir)
{
  glm_vec3_negate_to(position, dir);
  glm_normalize(dir);
}