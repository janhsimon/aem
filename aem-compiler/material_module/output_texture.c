#include "output_texture.h"

#include <cgltf/cgltf.h>

#include <stdio.h>

const char* compression_to_string(enum AEMTextureCompression compression)
{
  if (compression == AEMTextureCompression_BC7)
  {
    return "BC7";
  }
  else if (compression == AEMTextureCompression_BC5)
  {
    return "BC5";
  }

  return "None";
}

void print_output_textures(const OutputTexture* output_textures, uint32_t texture_count)
{
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const OutputTexture* texture = &output_textures[texture_index];

    printf("Output texture #%llu:\n", texture_index);

    printf("\tBase resolution: %u x %u\n", texture->base_width, texture->base_height);
    printf("\tSize: %u bytes\n", texture->data_size);
    printf("\tLevel count: %u\n", texture->level_count);
    printf("\tChannel count: %u\n", texture->channel_count);
    printf("\tCompression: %s\n", compression_to_string(texture->compression));
  }
}