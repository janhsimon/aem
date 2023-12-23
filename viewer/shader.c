#include "shader.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define SHADER_LOG_SIZE 512

GLuint load_shader(const char* filename, GLenum type)
{
  FILE* file = fopen(filename, "rb");
  assert(file);

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  fseek(file, 0, SEEK_SET);
  GLchar* source = malloc(length + 1); // Extra character for null-termination
  assert(source);
  fread(source, 1, length, file);
  fclose(file);

  source[length] = '\0'; // Null-terminate the source

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  free(source);

  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar log[SHADER_LOG_SIZE];
    glGetShaderInfoLog(shader, SHADER_LOG_SIZE, NULL, log);
    printf("Failed to compile shader:\n%s", log);
  }

  return shader;
}

GLuint generate_shader_program(GLuint vertex_shader, GLuint fragment_shader)
{
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);

  glLinkProgram(shader_program);

  GLint success;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar log[SHADER_LOG_SIZE];
    glGetProgramInfoLog(shader_program, SHADER_LOG_SIZE, NULL, log);
    printf("Failed to link shader program:\n%s", log);
  }

  return shader_program;
}

GLint get_uniform_location(GLuint shader_program, const char* name)
{
  GLint uniform_location = glGetUniformLocation(shader_program, name);
  if (shader_program < 0)
  {
    printf("Failed to get uniform location for %s", name);
  }

  return uniform_location;
}