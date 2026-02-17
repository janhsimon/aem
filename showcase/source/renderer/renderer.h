#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct Preferences;

bool load_renderer(struct Preferences* preferences, uint32_t screen_width, uint32_t screen_height);
void free_renderer();

void renderer_on_screen_resize(uint32_t screen_width, uint32_t screen_height);
void render_frame(float camera_near, float camera_far);