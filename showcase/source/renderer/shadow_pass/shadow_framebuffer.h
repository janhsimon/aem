#pragma once

#include <stdbool.h>

bool load_shadow_framebuffer();
void free_shadow_framebuffer();

void shadow_framebuffer_start_rendering();

unsigned int shadow_framebuffer_get_shadow_texture();
