#pragma once

#include <cglm/types.h>

#include <stdbool.h>

void load_enemy_state_die(enum EnemyState* state, const struct AEMModel* model, struct AEMAnimationMixer* mixer);
void enter_enemy_state_die();
void update_enemy_state_die(vec3 enemy_position,
                            vec3 enemy_forward,
                            float delta_time,
                            float* out_angle_delta,
                            bool* should_respawn);