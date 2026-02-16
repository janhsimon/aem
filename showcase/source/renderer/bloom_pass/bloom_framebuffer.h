#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BloomFramebufferPhase
{
  BloomFramebufferPhase_Downsample,
  BloomFramebufferPhase_Upsample
};

void bloom_framebuffer_get_texture_resolution(int texture_index, uint32_t* texture_width, uint32_t* texture_height);

bool load_bloom_framebuffer(uint32_t width, uint32_t height);
void free_bloom_framebuffer();

void bloom_framebuffer_start_rendering(uint32_t width,
                                       uint32_t height,
                                       int texture_index,
                                       enum BloomFramebufferPhase phase);

unsigned int bloom_framebuffer_get_texture(int texture_index, enum BloomFramebufferPhase phase);

unsigned int bloom_framebuffer_get_texture_count(enum BloomFramebufferPhase phase);
