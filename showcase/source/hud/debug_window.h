#pragma once

#include <stdbool.h>
#include <stdint.h>

struct Preferences;

bool has_debug_window_focus();

bool get_show_shadow_map_window();
bool get_show_view_space_normals_window();
bool get_show_ssao_window();
bool get_show_bloom_window();

void update_debug_window(struct Preferences* preferences, uint32_t screen_width, uint32_t screen_height);