#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool collide_capsule(vec3 base, vec3 tip, float radius, float* vertices, uint32_t* indices, uint32_t index_count);