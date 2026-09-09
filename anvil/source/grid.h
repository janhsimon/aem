#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool generate_grid();
void destroy_grid();

void draw_grid(const mat4 viewproj_matrix, bool fade_edges);