#pragma once

#include <cglm/types.h>

struct Preferences;

void camera_get_position(vec3 position);
void camera_set_position(vec3 position);

void camera_get_yaw_pitch_roll(float* yaw, float* pitch, float* roll);
void camera_set_yaw_pitch_roll(float yaw, float pitch, float roll);
void camera_add_yaw_pitch_roll(float yaw, float pitch, float roll);

void camera_reset_aim_punch();
void camera_add_aim_punch(float yaw, float pitch);
void camera_update_aim_punch(const struct Preferences* preferences, float recovery, float delta_time);

void camera_add_move(vec3 move);

enum CameraMode
{
  CameraMode_WithoutAimPunch,
  CameraMode_WithAimPunch
};

void camera_calc_forward();
void camera_get_forward(enum CameraMode mode, vec3 forward);

void camera_calc_rotation();
void camera_get_rotation(enum CameraMode mode, mat3 rotation);

void camera_calc_matrices(float aspect, float fov, float view_model_fov, float near, float far);
void camera_get_view_matrix(mat4 view_matrix);
void camera_get_proj_matrix(mat4 view_matrix);
void camera_get_view_model_proj_matrix(mat4 view_model_proj_matrix);
void camera_get_viewproj_matrix(mat4 view_matrix);

void camera_calc_frustum(const struct Preferences* preferences, float near, float far);
void camera_get_frustum_corners(vec4 frustum_corners[8]);
void camera_get_frustum_center(vec4 frustum_corners);
void camera_get_frustum_cascade_corners(int cascade_index, vec4 frustum_corners[8]);
void camera_get_frustum_cascade_center(int cascade_index, vec4 frustum_center);
