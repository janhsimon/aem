#pragma once

#include <stdbool.h>
#include <stdint.h>

void player_update(const struct Preferences* preferences, bool mouse_look, float delta_time, bool* moving);

float get_player_speed();