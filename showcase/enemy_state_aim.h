#pragma once

#include <cglm/types.h>

#include <stdbool.h>

void load_enemy_state_aim(const struct Preferences* preferences_,
                          enum EnemyState* state,
                          struct AEMAnimationMixer* mixer);
void enter_enemy_state_aim();
void update_enemy_state_aim(vec3 enemy_position,
                            vec3 enemy_forward,
                            bool player_visible,
                            float delta_time,
                            vec2 out_velocity,
                            float* out_angle_delta);