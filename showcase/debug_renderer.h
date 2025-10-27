#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_debug_renderer();

void add_debug_line(vec3 from, vec3 to);

void debug_render_capsule(vec3 from, vec3 to, float radius, vec3 color, float aspect, float fov);
void debug_render_lines(vec3 color, float aspect, float fov);
