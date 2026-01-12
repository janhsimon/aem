#pragma once

#include <cglm/types.h>

void load_enemy_state_flinch(enum EnemyState* state, const struct AEMModel* model, struct AEMAnimationMixer* mixer);
void enter_enemy_state_flinch();
void update_enemy_state_flinch();