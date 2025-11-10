#pragma once

#include <cglm/types.h>

void load_debug_renderer();
void free_debug_renderer();

void clear_debug_lines();
void add_debug_line(vec3 from, vec3 to);

void start_debug_rendering();

void debug_render_capsule(vec3 from, vec3 to, float radius, vec3 color);
void debug_render_lines(vec3 color, float aspect, float fov);