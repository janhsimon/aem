#pragma once

#include <cglm/types.h>

void clear_debug_lines();
void add_debug_line(vec3 from, vec3 to);

void render_debug_manager_capsule(vec3 from, vec3 to, float radius, vec3 color);
void render_debug_manager_lines(vec3 color);