#pragma once

#include <cglm/mat4.h>
#include <cglm/vec2.h>

void camera_tumble(vec2 delta);
void camera_pan(vec2 delta);
void camera_dolly(vec2 delta);

void reset_camera_pivot();

float* get_camera_position();
void calc_view_matrix(mat4 view_matrix);
void calc_proj_matrix(float aspect, float fov, mat4 proj_matrix);