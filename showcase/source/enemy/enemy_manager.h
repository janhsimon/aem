#pragma once

#include <stdbool.h>
#include <stdint.h>

struct Preferences;
struct ModelRenderInfo;
struct Enemy;

bool load_enemy_manager(const struct Preferences* preferences, uint32_t enemy_count);

void update_enemy_manager(float delta_time);

struct ModelRenderInfo* get_enemy_render_info();

uint32_t get_enemy_count();
struct Enemy* get_enemy(uint32_t enemy_index);

void free_enemy_manager();