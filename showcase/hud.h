#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_hud();

bool has_debug_window_focus();

void update_hud(uint32_t screen_width,
                uint32_t screen_height,
                bool debug_mode,
                float player_speed,
                struct Preferences* preferences);
void render_hud();

void free_hud();