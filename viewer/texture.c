#include "texture.h"

#include <aem/aem.h>

#include <glad/gl.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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
      glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA, level_width, level_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else
    {
      if (glGetError() != 0) { printf("ERROR PRE TEX\n"); }
      const GLenum format = (texture->compression == AEMTextureCompression_BC7) ? GL_COMPRESSED_RGBA_BPTC_UNORM_ARB :
                                                                                  GL_COMPRESSED_RG_RGTC2;
      glCompressedTexImage2D(GL_TEXTURE_2D, level_index, format, level_width, level_height, 0, level_size, level_data);
      if (glGetError() != 0) { printf("B\n"); }

    }

    level_data += level_size;
  }

  return tex;
}