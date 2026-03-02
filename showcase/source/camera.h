#pragma once

#include <cglm/types.h>

void camera_get_position(vec3 position);
void camera_set_position(vec3 position);

void camera_get_yaw_pitch_roll(float* yaw, float* pitch, float* roll);
void camera_set_yaw_pitch_roll(float yaw, float pitch, float roll);
void camera_add_yaw_pitch_roll(float yaw, float pitch, float roll);

void camera_add_recoil_yaw_pitch(float yaw, float pitch);
void camera_mul_recoil_yaw_pitch(float s);

void camera_add_move(vec3 move);

void camera_calc_forward();
void camera_get_forward_without_recoil(vec3 forward);
void camera_get_forward_with_recoil(vec3 forward);

void camera_calc_rotation();
void camera_get_rotation_without_recoil(mat3 rotation);
void camera_get_rotation_with_recoil(mat3 rotation);

void camera_calc_matrices(float aspect, float fov, float view_model_fov, float near, float far);
void camera_get_view_matrix(mat4 view_matrix);
void camera_get_proj_matrix(mat4 view_matrix);
void camera_get_view_model_proj_matrix(mat4 view_model_proj_matrix);
void camera_get_viewproj_matrix(mat4 view_matrix);

void camera_calc_frustum(float aspect, float fov, float near, float far);
void camera_get_frustum_corners(vec4 frustum_corners[8]);
void camera_get_frustum_center(vec4 frustum_corners);
