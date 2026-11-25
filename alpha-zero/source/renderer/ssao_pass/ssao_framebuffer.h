#pragma once

#include <stdbool.h>
#include <stdint.h>

bool load_ssao_framebuffer(uint32_t width, uint32_t height);
void free_ssao_framebuffer();

void ssao_framebuffer_on_screen_resize(uint32_t screen_width, uint32_t screen_height);

void ssao_framebuffer_start_rendering(int texture_index);

unsigned int ssao_framebuffer_get_texture(int texture_index);