#include "texture.h"

#include <aem/aem.h>

#include <glad/gl.h>

#include <math.h>
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

GLuint load_model_texture(const struct AEMModel* model, const struct AEMTexture* texture)
{
  // char filepath[AEM_STRING_SIZE * 2 + 2];
  // sprintf(filepath, "%s/%s", path, texture->filename);

  // struct AEMTextureData* texture_data = NULL;
  // if (aem_load_texture_data(filepath, &texture_data) != AEMResult_Success)
  //{
  //   printf("Failed to load model texture \"%s\"\n", filepath);
  //   return 0;
  // }

  // printf("Loaded model texture \"%s\" (Compression: %d)\n", filepath, texture->compression);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, aem_texture_wrap_mode_to_gl(texture->wrap_mode[0]));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, aem_texture_wrap_mode_to_gl(texture->wrap_mode[1]));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  const uint32_t level_count = 1 + floor(log2(max(texture->width, texture->height)));
  uint64_t offset = 0;
  for (uint32_t level_index = 0; level_index < level_count; ++level_index)
  {
    const uint32_t level_width = max(1, texture->width >> level_index);
    const uint32_t level_height = max(1, texture->height >> level_index);

    /*uint32_t mip_width, mip_height;
    uint64_t mip_size;
    void* mip_data;
    aem_get_texture_mip_data(texture_data, mip_index, &mip_width, &mip_height, &mip_size, &mip_data);*/

    /*const struct AEMLevel* level = aem_get_model_level(model, texture->first_level + level_index);
    void* level_data = aem_get_model_image_buffer_data_for_level(model, level);*/

    uint8_t* data = (uint8_t*)aem_get_model_image_buffer(model);

    // if (texture->compression == AEMTextureCompression_None)
    //{
    /*if (texture->channel_count == 1)
    {
      glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA, level_width, level_height, 0, GL_RED, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else if (texture->channel_count == 2)
    {
      glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA, level_width, level_height, 0, GL_RG, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else if (texture->channel_count == 3)
    {
      glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA, level_width, level_height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                   level_data);
    }
    else if (texture->channel_count == 4)
    {*/
    glTexImage2D(GL_TEXTURE_2D, level_index, GL_RGBA, level_width, level_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 &data[texture->offset + offset]);
    /*}
    else
    {
      assert(0);
    }*/

    offset += level_width * level_height * 4;
  }
  /*else if (texture->compression == AEMTextureCompression_BC5)
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, mip_index, GL_COMPRESSED_RG_RGTC2, mip_width, mip_height, 0, mip_size,
                           mip_data);
  }
  else
  {
    glCompressedTexImage2D(GL_TEXTURE_2D, mip_index, GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, mip_width, mip_height, 0,
                           mip_size, mip_data);
  }*/
  //}

  // aem_free_texture_data(texture_data);

  return tex;
}