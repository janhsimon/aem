#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_tracer_renderer();
void free_tracer_renderer();

void start_tracer_rendering();
void render_tracers(vec3* starts, vec3* ends, uint32_t tracer_count);