#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct AEMModel;

bool load_enemy(const struct Preferences* preferences, const struct AEMModel* model);

void update_enemy(float delta_time);
void prepare_enemy_rendering();

void get_enemy_world_matrix(mat4 world_matrix);

void debug_draw_enemy();

enum EnemyHitArea
{
  EnemyHitArea_None,
  EnemyHitArea_Head,
  EnemyHitArea_UpperTorso,
  EnemyHitArea_LowerTorso
};
enum EnemyHitArea is_enemy_hit(vec3 from, vec3 to);
void enemy_hurt(float damage, vec3 dir);

void free_enemy();