#pragma once

#include <cglm/types.h>

void load_enemy_state_aim(const struct Preferences* preferences_,
                          enum EnemyState* state,
                          struct AEMAnimationMixer* mixer);
void enter_enemy_state_aim();
void update_enemy_state_aim(struct EnemyStateInput enemy, struct EnemyStateOutput* output, float delta_time);