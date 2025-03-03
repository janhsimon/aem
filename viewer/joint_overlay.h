#pragma once

#include <cglm/mat4.h>

bool generate_joint_overlay();
void destroy_joint_overlay();

void draw_joint_overlay(mat4 world_matrix, mat4 viewproj_matrix, bool selected);