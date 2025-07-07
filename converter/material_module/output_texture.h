#pragma once

#include <aem/aem.h>

#include <stdint.h>

struct OutputTexture
{
  uint32_t base_width, base_height;
  uint32_t level_count;
  uint8_t* data;
  uint32_t data_size;
  enum AEMTextureCompression compression;
  uint32_t channel_count;
};
typedef struct OutputTexture OutputTexture;

void print_output_textures(const OutputTexture* output_textures, uint32_t texture_count);