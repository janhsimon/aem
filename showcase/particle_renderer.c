#include "particle_renderer.h"

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdlib.h>

#define FLOATS_PER_VERTEX 5 // 3 for position, 2 for uv
float vertices[FLOATS_PER_VERTEX * 4];

GLuint vao;
GLuint vbo;

GLuint tex;

bool load_particle_renderer()
{
  vertices[0 * FLOATS_PER_VERTEX + 0] = -0.5f;
  vertices[0 * FLOATS_PER_VERTEX + 1] = -0.5f;
  vertices[0 * FLOATS_PER_VERTEX + 2] = 0.0f;
  vertices[0 * FLOATS_PER_VERTEX + 3] = 0.0f;
  vertices[0 * FLOATS_PER_VERTEX + 4] = 0.0f;

  vertices[1 * FLOATS_PER_VERTEX + 0] = -0.5f;
  vertices[1 * FLOATS_PER_VERTEX + 1] = 0.5f;
  vertices[1 * FLOATS_PER_VERTEX + 2] = 0.0f;
  vertices[1 * FLOATS_PER_VERTEX + 3] = 0.0f;
  vertices[1 * FLOATS_PER_VERTEX + 4] = 1.0f;

  vertices[2 * FLOATS_PER_VERTEX + 0] = 0.5f;
  vertices[2 * FLOATS_PER_VERTEX + 1] = 0.5f;
  vertices[2 * FLOATS_PER_VERTEX + 2] = 0.0f;
  vertices[2 * FLOATS_PER_VERTEX + 3] = 1.0f;
  vertices[2 * FLOATS_PER_VERTEX + 4] = 1.0f;

  vertices[3 * FLOATS_PER_VERTEX + 0] = 0.5f;
  vertices[3 * FLOATS_PER_VERTEX + 1] = -0.5f;
  vertices[3 * FLOATS_PER_VERTEX + 2] = 0.0f;
  vertices[3 * FLOATS_PER_VERTEX + 3] = 1.0f;
  vertices[3 * FLOATS_PER_VERTEX + 4] = 0.0f;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, (void*)(sizeof(float) * 0));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, (void*)(sizeof(float) * 3));

  // Load the texture
  {
    int width, height, nrChannels;
    unsigned char* data = stbi_load("textures/muzzleflash1.png", &width, &height, &nrChannels, 4);
    if (!data)
    {
      return false;
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // Set basic texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
  }

  return true;
}

void free_particle_renderer()
{
  glDeleteTextures(1, &tex);
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void start_particle_rendering()
{
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(float) * FLOATS_PER_VERTEX, vertices);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
}

void render_particle()
{
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}