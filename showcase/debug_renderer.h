#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_debug_renderer();
void draw_line(vec3 from, vec3 to, vec3 color);
void set_line(int index, vec3 from, vec3 to, vec3 color);
void debug_render(float aspect, float fov);
