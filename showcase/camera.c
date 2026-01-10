#include "camera.h"

#include <cglm/cam.h>
#include <cglm/vec2.h>

#define PITCH_CLAMP 1.5533f // 89 deg in rad

static vec3 position = GLM_VEC3_ZERO_INIT;
static vec3 angles = GLM_VEC3_ZERO_INIT; // pitch, yaw, roll
static vec2 recoil = GLM_VEC2_ZERO_INIT;

void cam_get_position(vec3 position_)
{
  glm_vec3_copy(position, position_);
}

void cam_set_position(vec3 position_)
{
  glm_vec3_copy(position_, position);
}

void cam_calc_forward(enum CameraRotationMode mode, vec3 forward)
{
  vec3 final;

  if (mode == CameraRotationMode_WithoutRecoil)
  {
    glm_vec3_copy(angles, final);
  }
  else
  {
    final[0] = angles[0] + recoil[0];
    final[1] = angles[1] + recoil[1];
    final[2] = angles[2];
  }

  forward[0] = cosf(final[1]) * cosf(final[0]);
  forward[1] = sinf(final[0]);
  forward[2] = sinf(final[1]) * cosf(final[0]);

  glm_vec3_normalize(forward);
}

void cam_calc_rotation(mat3 rotation, enum CameraRotationMode mode)
{
  vec3 forward;
  cam_calc_forward(mode, forward);

  vec3 right;
  glm_vec3_cross(GLM_YUP, forward, right);
  glm_vec3_normalize(right);

  vec3 up;
  glm_vec3_cross(forward, right, up);

  glm_vec3_copy(right, rotation[0]);
  glm_vec3_copy(up, rotation[1]);
  glm_vec3_copy(forward, rotation[2]);
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

void calc_view_matrix(mat4 view_matrix)
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

void calc_proj_matrix(float aspect, float fov, float near, float far, mat4 proj_matrix)
{
  glm_perspective(glm_rad(fov), aspect, near, far, proj_matrix);
}