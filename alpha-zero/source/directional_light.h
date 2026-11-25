#pragma once

#include <cglm/types.h>

struct Preferences;

void directional_light_calc_viewproj(struct Preferences* preferences, float near, float far);

void directional_light_get_view_matrix(int cascade_index, mat4 view);
void directional_light_get_proj_matrix(int cascade_index, mat4 view);
void directional_light_get_viewproj_matrix(int cascade_index, mat4 view);