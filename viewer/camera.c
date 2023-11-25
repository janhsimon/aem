#include "camera.h"

#include <cglm/affine.h>
#include <cglm/cam.h>

static const float near = 0.1f;
static const float far = 1000.0f;
static const float fov = GLM_PI_4; // 45deg, vertical field of view in radians

static vec3 position = { 0.0f, 0.4f, -4.0f };
static vec3 pivot = { 0.0f, 0.4f, 0.0f };

void camera_tumble(vec2 delta)
{
  vec3 forward;
  glm_vec3_sub(pivot, position, forward);
  glm_vec3_normalize(forward);

  vec3 right;
  vec3 up = { 0.0f, 1.0f, 0.0f };
  glm_vec3_cross(up, forward, right);
  glm_vec3_cross(forward, right, up);

  mat4 tumble;
  glm_rotate_make(tumble, -delta[0], up); // Yaw
  glm_rotate(tumble, delta[1], right);    // Pitch

  vec3 move;
  glm_vec3_sub(position, pivot, move);
  glm_mat4_mulv(tumble, move, move);

  glm_vec3_add(pivot, move, position);
}

void camera_pan(vec2 delta)
{
  vec3 forward;
  glm_vec3_sub(pivot, position, forward);
  const float distance = glm_vec3_norm(forward);
  glm_vec3_normalize(forward);

  vec3 right;
  vec3 up = { 0.0f, 1.0f, 0.0f };
  glm_vec3_cross(up, forward, right);

  glm_vec3_scale(right, delta[0] * distance, right);
  glm_vec3_scale(up, delta[1] * distance, up);

  vec3 move;
  glm_vec3_add(up, right, move);
  glm_vec3_add(position, move, position);
  glm_vec3_add(pivot, move, pivot);
}

void camera_dolly(vec2 delta)
{
  vec3 forward;
  glm_vec3_sub(pivot, position, forward);
  const float distance = glm_vec3_norm(forward);
  glm_vec3_normalize(forward);

  const float move = distance - (float)delta[1] * distance / 10.0f;
  glm_vec3_scale(forward, move, forward);
  glm_vec3_sub(pivot, forward, position);
}

float* get_camera_position()
{
  return (float*)position;
}

void calc_view_matrix(mat4 view_matrix)
{
  glm_lookat(position, pivot, GLM_YUP, view_matrix);
}

void calc_proj_matrix(mat4 proj_matrix, float aspect)
{
  glm_perspective(fov, aspect, near, far, proj_matrix);
}