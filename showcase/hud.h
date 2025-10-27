#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_hud();

void update_hud(uint32_t screen_width, uint32_t screen_height, float player_speed);
void render_hud();

void free_hud();