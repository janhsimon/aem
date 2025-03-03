#pragma once

#include <cglm/mat4.h>

bool generate_wireframe_overlay();
void destroy_wireframe_overlay();

void begin_draw_wireframe_overlay(mat4 world_matrix, mat4 viewproj_matrix);
// void draw_wireframe_overlay(mat4 world_matrix, mat4 viewproj_matrix, bool selected);