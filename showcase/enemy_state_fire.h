#pragma once

#include <cglm/types.h>

void load_enemy_state_fire(const struct Preferences* preferences,
                           enum EnemyState* state,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer);
void enter_enemy_state_fire();
void update_enemy_state_fire(mat4 enemy_transform, float delta_time, vec2 out_velocity, float* out_angle_delta);