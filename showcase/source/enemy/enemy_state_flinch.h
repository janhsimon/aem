#pragma once

struct Enemy;
struct AEMModel;

void load_enemy_state_flinch(struct Enemy* enemy, const struct AEMModel* model);
void enter_enemy_state_flinch(struct Enemy* enemy);
void update_enemy_state_flinch(struct Enemy* enemy);