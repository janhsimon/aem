#include "model.h"

#include <math.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

uint32_t aem_get_model_texture_level_count(uint32_t texture_base_width, uint32_t texture_base_height)
{
  return 1 + floor(log2(MAX(texture_base_width, texture_base_height)));
}

void aem_get_model_texture_level_data(const struct AEMTexture* texture,
                                      uint32_t level_index,
                                      uint32_t* level_width,
                                      uint32_t* level_height,
                                      uint32_t* level_size)
{
  *level_width = MAX(1, texture->width >> level_index);
  *level_height = MAX(1, texture->height >> level_index);

  if (texture->compression == AEMTextureCompression_None)
  {
    *level_size = (*level_width) * (*level_height) * texture->channel_count;
  }
  else
  {
    const uint32_t block_width = (*level_width + 3) / 4;
    const uint32_t block_height = (*level_height + 3) / 4;
    *level_size = block_width * block_height * 16; // 16 bytes per BC5 or 7 block
  }
}