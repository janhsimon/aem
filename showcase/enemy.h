#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct AEMModel;

bool load_enemy(const struct AEMModel* model);

void update_enemy(float delta_time);
void prepare_enemy_rendering();

void debug_draw_enemy();

bool is_enemy_hit(vec3 from, vec3 to);
void enemy_die(vec3 dir);

void free_enemy();