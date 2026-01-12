#pragma once

#include <cglm/types.h>

#include <stdbool.h>

void load_enemy_state_walk(const struct Preferences* preferences,
                           enum EnemyState* state,
                           const struct AEMModel* model,
                           struct AEMAnimationMixer* mixer);
void enter_enemy_state_walk(bool instant);
void update_enemy_state_walk(vec3 enemy_position,
                             vec3 enemy_forward,
                             bool player_visible,
                             float delta_time,
                             vec2 out_velocity,
                             float* out_angle_delta);