#pragma once

#include <stdbool.h>
#include <stdint.h>

void player_update(float delta_time,
                   float* level_vertices,
                   uint32_t* level_indices,
                   uint32_t level_index_count,
                   bool* moving);