#pragma once

#include <cglm/types.h>

void cam_get_position(vec3 position);
void cam_get_orientation(mat3 orientation);

void cam_set_position(vec3 position);

void camera_add_yaw_pitch(float yaw, float pitch);
void camera_add_move(vec3 move);

void calc_view_matrix(mat4 view_matrix);
void calc_proj_matrix(float aspect, float fov, mat4 proj_matrix);