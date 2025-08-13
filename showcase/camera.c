#include "camera.h"

#include <cglm/cam.h>
#include <cglm/quat.h>

#define NEAR 0.01f
#define FAR 1000.0f
#define PITCH_CLAMP 1.5533f // 89 deg in rad

static vec3 position = GLM_VEC3_ZERO_INIT;
static mat3 orientation = GLM_MAT3_IDENTITY_INIT;
static float yaw, pitch = 0.0f; // Only used in camera_add_yaw_pitch()

void cam_set_position(float position_[3])
{
  glm_vec3_copy(position_, position);
}

void cam_get_position(float position_[3])
{
  glm_vec3_copy(position, position_);
}

void cam_get_orientation(float orientation_[9])
{
  glm_mat3_copy(orientation, (vec3*)orientation_);
}

void camera_add_yaw_pitch(float yaw_, float pitch_)
{
  yaw += yaw_;
  pitch += pitch_;

  pitch = glm_clamp(pitch, -PITCH_CLAMP, PITCH_CLAMP);

  // Yaw is applied first, rotating around world Y
  mat4 rot_yaw = GLM_MAT4_IDENTITY_INIT;
  glm_rotate_y(rot_yaw, -yaw, rot_yaw); // Negative because OpenGL uses right-handed system

  // Then pitch around local X
  mat4 rot_pitch = GLM_MAT4_IDENTITY_INIT;
  glm_rotate_x(rot_pitch, pitch, rot_pitch);

  // Combined rotation: orientation = rot_yaw * rot_pitch
  // (pitch is local, so post-multiplied)
  mat4 rot_combined;
  glm_mat4_mul(rot_yaw, rot_pitch, rot_combined);

  // Extract upper-left 3x3 part as the orientation basis
  glm_mat4_pick3(rot_combined, orientation);
}

void camera_add_move(float move[3])
{
  glm_mat3_mulv(orientation, move, move); // Transform move from local to camera space
  move[1] = 0.0f;                         // Flatten
  glm_vec3_add(position, move, position); // And add it to the camera position
}

void calc_view_matrix(mat4 view_matrix)
{
  // Find a point that is in front of the camera given its position and current orientation
  vec3 target;
  glm_vec3_add(position, orientation[2], target);

  glm_lookat(position, target, GLM_YUP, view_matrix);
}

void calc_proj_matrix(float aspect, float fov, mat4 proj_matrix)
{
  glm_perspective(glm_rad(fov), aspect, NEAR, FAR, proj_matrix);
}