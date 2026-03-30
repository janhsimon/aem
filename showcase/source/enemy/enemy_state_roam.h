#pragma once

#include <stdbool.h>

struct Enemy;
struct Preferences;
struct AEMModel;
struct EnemyStateOutput;

void load_enemy_state_roam(struct Enemy* enemy, const struct Preferences* preferences, const struct AEMModel* model);
void enter_enemy_state_roam(struct Enemy* enemy, bool instant);
void update_enemy_state_roam(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time);