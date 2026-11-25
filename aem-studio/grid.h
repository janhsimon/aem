#pragma once

#include <cglm/mat4.h>

bool generate_grid();
void destroy_grid();

void draw_grid(const mat4 viewproj_matrix);