#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glad/gl.h>

bool load_texture(const char* filepath, unsigned int* texture)
{
  int width, height, nrChannels;
  unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 4);
  if (!data)
  {
    return false;
  }

  glGenTextures(1, texture);
  glBindTexture(GL_TEXTURE_2D, *texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  // Set basic texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  stbi_image_free(data);

  return true;
}

void free_texture(unsigned int texture)
{
  glDeleteTextures(1, &texture);
}