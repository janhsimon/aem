#pragma once

#include <cglm/types.h>

void load_enemy_state_chase(const struct Preferences* preferences,
                           enum EnemyState* state,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer);
void enter_enemy_state_chase();
void update_enemy_state_chase(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time);