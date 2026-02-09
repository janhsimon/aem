#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct ModelRenderInfo;

enum Map
{
  Map_TestLevel,
  Map_Sponza
};

bool load_map(enum Map map);

void free_map();

uint32_t get_map_part_count();
struct ModelRenderInfo* get_map_part(uint32_t index);

uint32_t get_map_collision_index_count();
void get_map_collision_triangle(uint32_t first_index, vec3 v0, vec3 v1, vec3 v2);

void get_current_map_player_spawn(vec3 position, float* yaw);
void get_current_map_random_enemy_spawn(vec3 position, float* yaw);