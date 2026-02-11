#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_ssao_framebuffer(uint32_t screen_width, uint32_t screen_height);
void free_ssao_framebuffer();

void ssao_framebuffer_start_rendering(uint32_t screen_width, uint32_t screen_height);

unsigned int ssao_framebuffer_get_texture();