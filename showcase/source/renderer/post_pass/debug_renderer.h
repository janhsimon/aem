#pragma once

#include <cglm/types.h>

#include <stdint.h>

void load_debug_renderer();
void free_debug_renderer();

void start_debug_rendering_capsules();

void debug_render_capsule(vec3 from, vec3 to, float radius, vec3 color);
void debug_render_lines(vec3* vertices, uint32_t vertex_count, vec3 color);