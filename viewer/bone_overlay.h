#pragma once

#include <cglm/mat4.h>

bool generate_bone_overlay();
void destroy_bone_overlay();

void draw_bone_overlay(mat4 world_matrix, mat4 viewproj_matrix, bool selected);