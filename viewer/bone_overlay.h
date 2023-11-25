#pragma once

#include <cglm/mat4.h>

void generate_bone_overlay();
void draw_bone_overlay(mat4 world_matrix, mat4 viewproj_matrix);