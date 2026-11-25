#pragma once

struct Enemy;
struct Preferences;
struct AEMModel;
struct EnemyStateOutput;

void load_enemy_state_fire(struct Enemy* enemy, const struct Preferences* preferences, const struct AEMModel* model);
void enter_enemy_state_fire(struct Enemy* enemy);
void update_enemy_state_fire(struct Enemy* enemy,
                             struct EnemyStateOutput* output,
                             float delta_time);