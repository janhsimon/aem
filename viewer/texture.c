#include "texture.h"

#include <aem/aem.h>

#include <glad/gl.h>

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static GLuint aem_texture_wrap_mode_to_gl(enum AEMTextureWrapMode wrap_mode)
{
  if (wrap_mode == AEMTextureWrapMode_MirroredRepeat)
  {
    return GL_MIRRORED_REPEAT;
  }
  else if (wrap_mode == AEMTextureWrapMode_ClampToEdge)
  {
    return GL_CLAMP_TO_EDGE;
  }

  return GL_REPEAT;
}

static GLenum aem_texture_channel_count_to_gl_uncompressed_internal_format(uint32_t channel_count)
{
  if (channel_count == 2)
  {
    return GL_RG8;
  }
  else if (channel_count == 4)
  {
    return GL_RGBA8;
  }

  assert(false);
  return 0;
}

static GLenum aem_texture_channel_count_to_gl_uncompressed_format(uint32_t channel_count)
{
  if (channel_count == 2)
  {
    return GL_RG;
  }
  else if (channel_count == 4)
  {
    return GL_RGBA;
  }

  assert(false);
  return 0;
}

static GLenum aem_texture_to_gl_compressed_internal_format(const struct AEMTexture* texture)
{
  assert(texture->compression != AEMTextureCompression_None);

  if (texture->compression == AEMTextureCompression_BC7)
  {
    assert(texture->channel_count == 4);
    return GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
  }
  else if (texture->compression == AEMTextureCompression_BC5)
  {
    assert(texture->channel_count == 2);
    return GL_COMPRESSED_RG_RGTC2;
  }

  assert(false);
  return 0;
}

GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture)
{
  const uint8_t* data = (uint8_t*)aem_get_model_image_buffer(model);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, aem_texture_wrap_mode_to_gl(texture->wrap_mode[0]));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, aem_texture_wrap_mode_to_gl(texture->wrap_mode[1]));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  const uint8_t* level_data = &data[texture->offset];
  const uint32_t level_count = aem_get_model_texture_level_count(texture->width, texture->height);
  for (uint32_t level_index = 0; level_index < level_count; ++level_index)
  {
    uint32_t level_width, level_height;
    uint32_t level_size;
    aem_get_model_texture_level_data(texture, level_index, &level_width, &level_height, &level_size);

    if (texture->compression == AEMTextureCompression_None)
    {
      const GLenum internal_format =
        aem_texture_channel_count_to_gl_uncompressed_internal_format(texture->channel_count);
      const GLenum format = aem_texture_channel_count_to_gl_uncompressed_format(texture->channel_count);

      glTexImage2D(GL_TEXTURE_2D, level_index, internal_format, level_width, level_height, 0, format, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else
    {
      const GLenum internal_format = aem_texture_to_gl_compressed_internal_format(texture);

      glCompressedTexImage2D(GL_TEXTURE_2D, level_index, internal_format, level_width, level_height, 0, level_size,
                             level_data);
    }

    level_data += level_size;
  }

  return tex;
}