#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

bool load_hud_damage_indicator();

void hud_damage_indicate(vec3 damage_direction);

void update_hud_damage_indicator(struct ImDrawList* draw_list,
                                 uint32_t half_screen_width,
                                 uint32_t half_screen_height,
                                 float delta_time);

void free_hud_damage_indicator();