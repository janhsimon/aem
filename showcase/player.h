#pragma once

#include <stdbool.h>
#include <stdint.h>

void player_update(float delta_time, bool* moving);

float get_player_speed();