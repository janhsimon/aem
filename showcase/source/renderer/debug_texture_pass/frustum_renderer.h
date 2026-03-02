#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_frustum_renderer();
void free_frustum_renderer();

void start_frustum_rendering();
void render_frustum(float aspect, float fov, float near, float far);