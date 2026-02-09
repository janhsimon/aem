#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_particle_renderer();
void free_particle_renderer();

void start_particle_rendering();
void render_particles(vec3* positions,
                      float* scales,
                      float* opacities,
                      uint32_t particle_count,
                      bool additive,
                      vec3 tint,
                      uint32_t texture_index);