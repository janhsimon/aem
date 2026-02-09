#pragma once

#include <stdbool.h>

bool load_shadow_framebuffer();
void free_shadow_framebuffer();

void shadow_framebuffer_start_rendering();

void shadow_framebuffer_bind_shadow_texture(int slot);
