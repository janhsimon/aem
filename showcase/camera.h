#pragma once

#include <cglm/mat4.h>

void cam_get_position(float position_[3]);
void cam_get_orientation(float orientation_[9]);

void cam_set_position(float position[3]);

void camera_add_yaw_pitch(float yaw, float pitch);
void camera_add_move(float move[3]);

void calc_view_matrix(mat4 view_matrix);
void calc_proj_matrix(float aspect, float fov, mat4 proj_matrix);