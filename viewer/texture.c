#include "texture.h"

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

GLuint load_texture(const char* filename)
{
  int width, height, channels;
  stbi_uc* data = stbi_load(filename, &width, &height, &channels, 0);
  if (!data)
  {
    printf("Failed to load texture %s\n", filename);
    return 0;
  }

  printf("Loaded texture \"%s\" (%dx%dx%d)\n", filename, width, height, channels);

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLenum format = GL_RED;
  if (channels == 3u)
  {
    format = GL_RGB;
  }
  else if (channels == 4u)
  {
    format = GL_RGBA;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  return texture;
}