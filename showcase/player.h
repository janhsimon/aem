#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

void player_update(const struct Preferences* preferences, bool mouse_look, float delta_time, bool* moving);

void get_player_position(vec3 position);
void get_player_velocity(vec3 velocity);
float calc_angle_delta_towards_player(vec3 from_position, vec3 from_forward); // In degrees
bool get_player_grounded();