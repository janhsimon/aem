#include "shader.h"

#include <util/util.h>

#include <stdio.h>
#include <stdlib.h>

#define SHADER_LOG_SIZE 512

bool load_shader(const char* filename, GLenum type, GLuint* shader)
{
  long length;
  GLchar* source = (GLchar*)load_text_file(filename, &length);
  if (!source)
  {
    printf("Failed to open shader file: \"%s\"\n", filename);
    return false;
  }

  *shader = glCreateShader(type);

  const GLchar* sources[] = { source };
  glShaderSource(*shader, 1, sources, (GLint*)&length);
  free(source);

  glCompileShader(*shader);

  GLint success;
  glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    GLchar log[SHADER_LOG_SIZE];
    glGetShaderInfoLog(*shader, SHADER_LOG_SIZE, NULL, log);
    printf("Failed to compile shader:\n%s", log);
    return false;
  }

  return true;
}

bool generate_shader_program(GLuint vertex_shader, GLuint fragment_shader, GLuint* shader_program)
{
  *shader_program = glCreateProgram();
  glAttachShader(*shader_program, vertex_shader);
  glAttachShader(*shader_program, fragment_shader);

  glLinkProgram(*shader_program);

  GLint success;
  glGetProgramiv(*shader_program, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLchar log[SHADER_LOG_SIZE];
    glGetProgramInfoLog(*shader_program, SHADER_LOG_SIZE, NULL, log);
    printf("Failed to link shader program:\n%s", log);
    return false;
  }

  return true;
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