#pragma once

#include <cglm/types.h>

#include <stdbool.h>

void load_enemy_state_roam(const struct Preferences* preferences,
                           enum EnemyState* state,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer);
void enter_enemy_state_roam(bool instant);
void update_enemy_state_roam(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time);