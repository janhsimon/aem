#include "util.h"

#include <aem/model.h>

#include <assert.h>

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

static void aem_texture_to_gl_formats(const struct AEMTexture* texture, GLenum* internal_format, GLenum* format)
{
  if (texture->compression == AEMTextureCompression_None)
  {
    if (texture->channel_count == 2)
    {
      *internal_format = GL_RG8;
      *format = GL_RG;
    }
    else if (texture->channel_count == 4)
    {
      *internal_format = GL_RGBA8;
      *format = GL_RGBA;
    }
    else
    {
      assert(false);
    }
  }
  else
  {
    if (texture->compression == AEMTextureCompression_BC5)
    {
      assert(texture->channel_count == 2);
      *internal_format = GL_COMPRESSED_RG_RGTC2;
    }
    else if (texture->compression == AEMTextureCompression_BC7)
    {
      assert(texture->channel_count == 4);
      *internal_format = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
    }
    else
    {
      assert(false);
    }
  }
}

GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture)
{
  const uint8_t* data = (uint8_t*)aem_get_model_image_buffer(model);

  GLuint texture_handle;
  glGenTextures(1, &texture_handle);
  glBindTexture(GL_TEXTURE_2D, texture_handle);
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

    GLenum internal_format, format;
    aem_texture_to_gl_formats(texture, &internal_format, &format);

    if (texture->compression == AEMTextureCompression_None)
    {
      glTexImage2D(GL_TEXTURE_2D, level_index, internal_format, level_width, level_height, 0, format, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else
    {
      glCompressedTexImage2D(GL_TEXTURE_2D, level_index, internal_format, level_width, level_height, 0, level_size,
                             level_data);
    }

    level_data += level_size;
  }

  return texture_handle;
}