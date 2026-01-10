#pragma once

#include <cglm/types.h>

void cam_get_position(vec3 position);
void cam_set_position(vec3 position);

void camera_get_yaw_pitch_roll(float* yaw, float* pitch, float* roll);
void camera_set_yaw_pitch_roll(float yaw, float pitch, float roll);
void camera_add_yaw_pitch_roll(float yaw, float pitch, float roll);

void camera_add_recoil_yaw_pitch(float yaw, float pitch);
void camera_mul_recoil_yaw_pitch(float s);

void camera_add_move(vec3 move);

enum CameraRotationMode
{
  CameraRotationMode_WithoutRecoil,
  CameraRotationMode_WithRecoil
};
void cam_calc_forward(enum CameraRotationMode mode, vec3 forward);
void cam_calc_rotation(mat3 rotation, enum CameraRotationMode mode);

void calc_view_matrix(mat4 view_matrix);
void calc_proj_matrix(float aspect, float fov, float near, float far, mat4 proj_matrix);