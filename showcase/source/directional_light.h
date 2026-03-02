#pragma once

#include <cglm/types.h>

struct Preferences;

void directional_light_calc_viewproj(struct Preferences* preferences,
                                     vec4 frustum_corners[8],
                                     vec4 frustum_center);

void directional_light_get_view_matrix(mat4 view);
void directional_light_get_proj_matrix(mat4 view);
void directional_light_get_viewproj_matrix(mat4 view);