#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

void player_update(const struct Preferences* preferences, bool mouse_look, float delta_time, bool* moving);

void get_player_position(vec3 position);
void get_player_velocity(vec3 velocity);
float calc_angle_delta_towards_player(vec3 from_position, vec3 from_forward); // In degrees

bool get_player_grounded();
void player_jump();

float get_player_health();

bool is_player_hit(vec3 from, vec3 to);
void player_hurt(float damage, vec3 dir);