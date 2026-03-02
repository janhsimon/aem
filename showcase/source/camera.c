#include "camera.h"

#include <cglm/cam.h>
#include <cglm/frustum.h>
#include <cglm/mat3.h>
#include <cglm/vec2.h>

#define PITCH_CLAMP 1.5533f // 89 deg in rad

static vec3 position = GLM_VEC3_ZERO_INIT;
static vec3 angles = GLM_VEC3_ZERO_INIT; // pitch, yaw, roll
static vec2 recoil = GLM_VEC2_ZERO_INIT;

static vec3 forward_without_recoil, forward_with_recoil;
static mat3 rotation_without_recoil, rotation_with_recoil;

static mat4 view_matrix, proj_matrix, view_model_proj_matrix, viewproj_matrix;

static vec4 frustum_corners[8];
static vec4 frustum_center;

void camera_get_position(vec3 position_)
{
  glm_vec3_copy(position, position_);
}

void camera_set_position(vec3 position_)
{
  glm_vec3_copy(position_, position);
}

static void angles_to_direction(vec3 angles_, vec3 direction)
{
  direction[0] = cosf(angles_[1]) * cosf(angles_[0]);
  direction[1] = sinf(angles_[0]);
  direction[2] = sinf(angles_[1]) * cosf(angles_[0]);

  glm_normalize(direction);
}

void camera_calc_forward()
{
  angles_to_direction(angles, forward_without_recoil);

  vec3 angles_with_recoil;
  angles_with_recoil[0] = angles[0] + recoil[0];
  angles_with_recoil[1] = angles[1] + recoil[1];
  angles_with_recoil[2] = angles[2];

  angles_to_direction(angles_with_recoil, forward_with_recoil);
}

void camera_get_forward_without_recoil(vec3 forward)
{
  glm_vec3_copy(forward_without_recoil, forward);
}

void camera_get_forward_with_recoil(vec3 forward)
{
  glm_vec3_copy(forward_with_recoil, forward);
}

static void forward_to_rotation_matrix(vec3 forward, mat3 rotation)
{
  vec3 right;
  glm_vec3_cross(GLM_YUP, forward, right);
  glm_vec3_normalize(right);

  vec3 up;
  glm_vec3_cross(forward, right, up);

  glm_vec3_copy(right, rotation[0]);
  glm_vec3_copy(up, rotation[1]);
  glm_vec3_copy(forward, rotation[2]);
}

void camera_calc_rotation()
{
  vec3 forward_without_recoil, forward_with_recoil;
  camera_get_forward_without_recoil(forward_without_recoil);
  camera_get_forward_with_recoil(forward_with_recoil);

  forward_to_rotation_matrix(forward_without_recoil, rotation_without_recoil);
  forward_to_rotation_matrix(forward_with_recoil, rotation_with_recoil);
}

void camera_get_rotation_without_recoil(mat3 rotation)
{
  glm_mat3_copy(rotation_without_recoil, rotation);
}

void camera_get_rotation_with_recoil(mat3 rotation)
{
  glm_mat3_copy(rotation_with_recoil, rotation);
}

void camera_get_yaw_pitch_roll(float* yaw, float* pitch, float* roll)
{
  *pitch = angles[0];
  *yaw = angles[1];
  *roll = angles[2];
}

void camera_set_yaw_pitch_roll(float yaw, float pitch, float roll)
{
  angles[0] = pitch;
  angles[1] = yaw;
  angles[2] = roll;
}

void camera_add_yaw_pitch_roll(float yaw, float pitch, float roll)
{
  angles[0] -= pitch;
  angles[1] += yaw;
  angles[2] += roll;

  // Clamp the pitch
  if (angles[0] < -PITCH_CLAMP)
  {
    angles[0] = -PITCH_CLAMP;
  }
  else if (angles[0] > PITCH_CLAMP)
  {
    angles[0] = PITCH_CLAMP;
  }
}

void camera_add_recoil_yaw_pitch(float yaw, float pitch)
{
  recoil[1] += yaw;
  recoil[0] -= pitch;
}

void camera_mul_recoil_yaw_pitch(float s)
{
  glm_vec2_scale(recoil, s, recoil);
}

void camera_add_move(vec3 move)
{
  glm_vec3_add(position, move, position);
}

void camera_calc_matrices(float aspect, float fov, float view_model_fov, float near, float far)
{
  // View matrix
  {
    vec3 final;
    final[0] = angles[0] + recoil[0]; // Pitch
    final[1] = angles[1] + recoil[1]; // Yaw
    final[2] = angles[2];             // Roll

    // Forward vector from yaw + pitch
    vec3 forward = { cosf(final[1]) * cosf(final[0]), sinf(final[0]), sinf(final[1]) * cosf(final[0]) };
    glm_vec3_normalize(forward);

    // Right vector
    vec3 right;
    glm_vec3_cross(GLM_YUP, forward, right);
    glm_vec3_normalize(right);

    // Up vector
    vec3 up;
    glm_vec3_cross(forward, right, up);
    glm_vec3_normalize(up);

    // Roll
    if (final[2] != 0.0f)
    {
      glm_vec3_rotate(right, final[2], forward);
      glm_vec3_rotate(up, final[2], forward);
    }

    // Look target
    vec3 target;
    glm_vec3_add(position, forward, target);

    glm_lookat(position, target, up, view_matrix);
  }

  // Projection matrices
  {
    glm_perspective(glm_rad(fov), aspect, near, far, proj_matrix);
    glm_perspective(glm_rad(view_model_fov), aspect, near, far, view_model_proj_matrix);
  }

  glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);
}

void camera_get_view_matrix(mat4 view_matrix_)
{
  glm_mat4_copy(view_matrix, view_matrix_);
}

void camera_get_proj_matrix(mat4 proj_matrix_)
{
  glm_mat4_copy(proj_matrix, proj_matrix_);
}

void camera_get_view_model_proj_matrix(mat4 view_model_proj_matrix_)
{
  glm_mat4_copy(view_model_proj_matrix, view_model_proj_matrix_);
}

void camera_get_viewproj_matrix(mat4 viewproj_matrix_)
{
  glm_mat4_copy(viewproj_matrix, viewproj_matrix_);
}

void camera_calc_frustum(float aspect, float fov, float near, float far)
{
  mat4 inv_viewproj;
  glm_mat4_inv(viewproj_matrix, inv_viewproj);

  glm_frustum_corners(inv_viewproj, frustum_corners);
  glm_frustum_center(frustum_corners, frustum_center);
}

void camera_get_frustum_corners(vec4 frustum_corners_[8])
{
  for (uint32_t i = 0; i < 8; ++i)
  {
    glm_vec4_copy(frustum_corners[i], frustum_corners_[i]);
  }
}

void camera_get_frustum_center(vec4 frustum_center_)
{
  glm_vec4_copy(frustum_center, frustum_center_);
}