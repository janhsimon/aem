#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_map();

void draw_map_opaque();
void draw_map_transparent();

void free_map();

uint32_t get_map_collision_index_count();
void get_map_collision_triangle(uint32_t first_index, vec3 v0, vec3 v1, vec3 v2);