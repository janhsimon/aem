#pragma once

#include <stdbool.h>

struct Preferences;

bool load_shadow_framebuffer(const struct Preferences* preferences);
void free_shadow_framebuffer();

void shadow_framebuffer_start_rendering(const struct Preferences* preferences, int cascade_index);

unsigned int shadow_framebuffer_get_shadow_cascade(int cascade_index);
