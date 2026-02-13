#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_ssao_framebuffer(uint32_t width, uint32_t height);
void free_ssao_framebuffer();

void ssao_framebuffer_start_rendering(uint32_t width, uint32_t height, int texture_index);

unsigned int ssao_framebuffer_get_texture(int texture_index);