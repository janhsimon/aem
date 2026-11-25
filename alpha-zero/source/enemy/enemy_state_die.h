#pragma once

struct Enemy;
struct EnemyStateOutput;

void load_enemy_state_die(struct Enemy* enemy);
void enter_enemy_state_die(struct Enemy* enemy);
void update_enemy_state_die(struct Enemy* enemy, struct EnemyStateOutput* output, float delta_time);