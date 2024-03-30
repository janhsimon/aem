#include "camera.h"

#include <cglm/affine.h>
#include <cglm/cam.h>

static const float near = 0.1f;
static const float far = 1000.0f;

static vec3 position = { 0.0f, 0.4f, -4.0f };
static vec3 pivot = { 0.0f, 0.4f, 0.0f };
static vec3 up = { 0.0f, 1.0f, 0.0f };

void camera_tumble(vec2 delta)
{
  vec3 forward;
  glm_vec3_sub(pivot, position, forward);
  glm_vec3_normalize(forward);

  vec3 right;
  vec3 temp_up = { 0.0f, 1.0f, 0.0f };
  glm_vec3_cross(temp_up, forward, right);
  glm_vec3_cross(forward, right, up);
  glm_vec3_normalize(up);

  // Limit the pitch angle
  const float dot = glm_dot(forward, temp_up);
  if ((dot <= -0.99f && delta[1] > 0.0f) || (dot >= 0.99f && delta[1] < 0.0f))
  {
    delta[1] = 0.0f;
  }

  mat4 tumble;
  glm_rotate_make(tumble, -delta[0], up); // Yaw
  glm_rotate(tumble, delta[1], right);    // Pitch

  vec3 move;
  glm_vec3_sub(position, pivot, move);

  glm_mat4_mulv3(tumble, move, 0.0f, move);
  glm_vec3_add(pivot, move, position);
}

void camera_pan(vec2 delta)
{
  vec3 forward;
  glm_vec3_sub(pivot, position, forward);
  const float distance = glm_vec3_norm(forward);
  glm_vec3_normalize(forward);

  vec3 right;
  glm_vec3_cross(up, forward, right);
  glm_vec3_normalize(right);

  vec3 x, y;
  glm_vec3_scale(right, delta[0] * distance, x);
  glm_vec3_scale(up, delta[1] * distance, y);

  vec3 move;
  glm_vec3_add(y, x, move);
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

void reset_camera_pivot()
{
  pivot[0] = 0.0f;
  pivot[1] = 0.4f;
  pivot[2] = 0.0f;
}
float* get_camera_position()
{
  return (float*)position;
}

void calc_view_matrix(mat4 view_matrix)
{
  glm_lookat(position, pivot, GLM_YUP, view_matrix);
}

void calc_proj_matrix(float aspect, float fov, mat4 proj_matrix)
{
  glm_perspective(fov, aspect, near, far, proj_matrix);
}