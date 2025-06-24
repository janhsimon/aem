#include "texture_compressor.h"

#include "output_texture.h"

#include <aem/aem.h>

#include <ktx/ktx.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>

static ktx_uint32_t aem_texture_compression_to_ktx_texture_format(enum AEMTextureCompression compression)
{
  if (compression == AEMTextureCompression_BC7)
  {
    return /*VK_FORMAT_R8G8B8A8_UNORM*/ 37;
  }
  else if (compression == AEMTextureCompression_BC5)
  {
    return /*VK_FORMAT_R8G8_UNORM*/ 16;
  }
  else
  {
    assert(false);
    return 0;
  }
}

static ktx_bool_t aem_texture_compression_to_ktx_uastc_flag(enum AEMTextureCompression compression)
{
  if (compression == AEMTextureCompression_BC7)
  {
    return KTX_TRUE;
  }
  else if (compression == AEMTextureCompression_BC5)
  {
    return KTX_FALSE;
  }
  else
  {
    assert(false);
    return 0;
  }
}

static ktx_transcode_fmt_e aem_texture_compression_to_ktx_transcode_format(enum AEMTextureCompression compression)
{
  if (compression == AEMTextureCompression_BC7)
  {
    return KTX_TTF_BC7_RGBA;
  }
  else if (compression == AEMTextureCompression_BC5)
  {
    return KTX_TTF_BC5_RG;
  }
  else
  {
    assert(false);
    return 0;
  }
}

static ktx_size_t
get_level_size(uint32_t base_width, uint32_t base_height, uint32_t level_index, enum AEMTextureCompression compression)
{
  const ktx_uint32_t level_width = max(base_width >> level_index, 1);
  const ktx_uint32_t level_height = max(base_height >> level_index, 1);

  if (compression == AEMTextureCompression_BC5)
  {
    return level_width * level_height * 2;
  }

  return level_width * level_height * 4;
}

void compress_texture(OutputTexture* texture, enum AEMTextureCompression compression)
{
  texture->compression = compression;

  // Create a KTX texture for the render texture
  ktxTexture2* ktx_texture = NULL;
  {
    ktxTextureCreateInfo createInfo;
    memset(&createInfo, 0, sizeof(createInfo));
    createInfo.vkFormat = aem_texture_compression_to_ktx_texture_format(texture->compression);
    createInfo.baseWidth = (ktx_uint32_t)texture->base_width;
    createInfo.baseHeight = (ktx_uint32_t)texture->base_height;
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = (ktx_uint32_t)texture->level_count;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;

    const KTX_error_code result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &ktx_texture);
    if (result != KTX_SUCCESS)
    {
      fprintf(stderr, "Failed to create texture: %s\n", ktxErrorString(result));
      return;
    }
  }

  // Fill each mip level of the KTX texture
  {
    // Fill in each mip level
    ktx_size_t source_offset = 0;
    for (uint32_t level_index = 0; level_index < (ktx_uint32_t)texture->level_count; ++level_index)
    {
      const ktx_size_t level_size =
        get_level_size(texture->base_width, texture->base_height, level_index, texture->compression);

      const KTX_error_code result = ktxTexture_SetImageFromMemory(ktxTexture(ktx_texture), level_index, 0, 0,
                                                                  texture->data + source_offset, level_size);
      if (result != KTX_SUCCESS)
      {
        fprintf(stderr, "Failed to set image from memory for texture: %s\n", ktxErrorString(result));
        return;
      }

      source_offset += level_size;
    }
  }

  free(texture->data);

  // Compress the KTX texture
  {
    // Compress using BasisU in BC7 mode (super-fast preset)
    ktxBasisParams params;
    memset(&params, 0, sizeof(params));
    params.structSize = sizeof(params),
    params.compressionLevel = 1; // Fastest (1 = fastest, 5 = slowest/highest quality)
    params.qualityLevel = 128;   // 0–255 (128 = balanced)
    params.uastc = aem_texture_compression_to_ktx_uastc_flag(texture->compression);

    const KTX_error_code result = ktxTexture2_CompressBasisEx(ktx_texture, &params);
    if (result != KTX_SUCCESS)
    {
      fprintf(stderr, "Failed to compress texture: %s\n", ktxErrorString(result));
      return;
    }
  }

  // Transcode the texture
  if (ktxTexture2_NeedsTranscoding(ktx_texture))
  {
    const ktx_transcode_fmt_e format = aem_texture_compression_to_ktx_transcode_format(compression);
    const KTX_error_code result = ktxTexture2_TranscodeBasis(ktx_texture, format, 0);
    if (result != KTX_SUCCESS)
    {
      fprintf(stderr, "Failed to transcode texture: %s\n", ktxErrorString(result));
      return;
    }
  }

  texture->data_size = ktxTexture_GetDataSize(ktxTexture(ktx_texture));
  texture->data = malloc(texture->data_size);
  assert(texture->data);

  // Copy the compressed texture memory to the output texture
  {
    uint32_t destination_offset = 0;
    for (uint32_t level_index = 0; level_index < texture->level_count; ++level_index)
    {
      ktx_uint8_t* source_data = ktxTexture_GetData(ktxTexture(ktx_texture));

      // Calculate the source offset
      ktx_size_t source_offset;
      {
        const KTX_error_code result = ktxTexture2_GetImageOffset(ktx_texture, level_index, 0, 0, &source_offset);
        if (result != KTX_SUCCESS)
        {
          fprintf(stderr, "Failed to get image offset for texture: %s\n", ktxErrorString(result));
          return;
        }
      }

      const ktx_size_t level_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), level_index);
      memcpy(&texture->data[destination_offset], source_data + source_offset, level_size);
      destination_offset += level_size;
    }

    assert(destination_offset == texture->data_size);
  }

  // Clean up
  ktxTexture_Destroy(ktxTexture(ktx_texture));
}