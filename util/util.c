#include "util.h"

#include <cglm/util.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void indent(unsigned int indent_count, const char* marker, const char* last_marker)
{
  for (unsigned int indent = 0; indent < indent_count; ++indent)
  {
    if (!last_marker || indent < indent_count - 1)
    {
      printf("%s", marker);
    }
    else
    {
      printf("%s", last_marker);
    }
  }
}

void print_checkbox(const char* title, bool condition)
{
  printf("%s [%c]", title, condition ? 'X' : ' ');
}

int is_mat4_identity(const float* mat)
{
  if (!glm_eq(mat[0], 1.0f) || !glm_eq(mat[5], 1.0f) || !glm_eq(mat[10], 1.0f) || !glm_eq(mat[15], 1.0f))
  {
    return 0;
  }

  if (!glm_eq(mat[1], 0.0f) || !glm_eq(mat[2], 0.0f) || !glm_eq(mat[3], 0.0f))
  {
    return 0;
  }

  if (!glm_eq(mat[4], 0.0f) || !glm_eq(mat[6], 0.0f) || !glm_eq(mat[7], 0.0f))
  {
    return 0;
  }

  if (!glm_eq(mat[8], 0.0f) || !glm_eq(mat[9], 0.0f) || !glm_eq(mat[11], 0.0f))
  {
    return 0;
  }

  if (!glm_eq(mat[12], 0.0f) || !glm_eq(mat[13], 0.0f) || !glm_eq(mat[14], 0.0f))
  {
    return 0;
  }

  return 1;
}

char* filename_from_filepath(char* filepath)
{
  char* trim = filepath;
  while (*filepath != '\0')
  {
    if (*filepath == '\\' || *filepath == '/')
    {
      trim = filepath + 1;
    }
    ++filepath;
  }

  return trim;
}

char* path_from_filepath(char* filepath)
{
  int length = 0;
  int offset = 0;
  while (filepath[offset] != '\0')
  {
    if (filepath[offset] == '\\' || filepath[offset] == '/')
    {
      length = offset;
    }
    ++offset;
  }

  char* path = malloc(length + 1);
  assert(path);
  memcpy(path, filepath, length);
  path[length] = '\0';
  return path;
}

char* basename_from_filename(char* filename)
{
  int length = 0;
  int offset = 0;
  while (filename[offset] != '\0')
  {
    if (filename[offset] == '.')
    {
      length = offset;
    }
    ++offset;
  }

  char* path = malloc(length + 1);
  assert(path);
  memcpy(path, filename, length);
  path[length] = '\0';
  return path;
}