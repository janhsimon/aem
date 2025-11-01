#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_hud();

void update_hud(uint32_t screen_width,
                uint32_t screen_height,
                bool debug_mode,
                float player_speed,
                vec3 light_dir,
                bool* debug_render);
void render_hud();

void free_hud();