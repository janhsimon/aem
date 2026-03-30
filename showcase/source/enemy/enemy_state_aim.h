#pragma once

struct Enemy;
struct Preferences;
struct EnemyStateOutput;

void load_enemy_state_aim(struct Enemy* enemy, const struct Preferences* preferences);
void enter_enemy_state_aim(struct Enemy* enemy);
void update_enemy_state_aim(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time);