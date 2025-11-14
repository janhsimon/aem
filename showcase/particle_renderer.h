#pragma once

#include <cglm/types.h>

#include <stdbool.h>

bool load_particle_renderer();
void free_particle_renderer();

void start_particle_rendering();
void render_particle();