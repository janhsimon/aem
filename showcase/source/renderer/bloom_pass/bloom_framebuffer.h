#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BloomFramebufferPhase
{
  BloomFramebufferPhase_Downsample,
  BloomFramebufferPhase_Upsample
};

bool load_bloom_framebuffer(uint32_t screen_width, uint32_t screen_height);
void free_bloom_framebuffer();

void bloom_framebuffer_on_screen_resize(uint32_t screen_width, uint32_t screen_height);

void bloom_framebuffer_start_rendering(int texture_index, enum BloomFramebufferPhase phase);

unsigned int bloom_framebuffer_get_texture(int texture_index, enum BloomFramebufferPhase phase);
unsigned int bloom_framebuffer_get_texture_count(enum BloomFramebufferPhase phase);
void bloom_framebuffer_get_texture_resolution(int texture_index, uint32_t* texture_width, uint32_t* texture_height);