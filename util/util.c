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

char* path_from_filepath(const char* filepath)
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

char* extension_from_filepath(char* filepath)
{
  char* extension = filepath;
  while (*filepath != '\0')
  {
    if (*filepath == '.')
    {
      extension = filepath + 1;
    }
    ++filepath;
  }

  return extension;
}

char* load_text_file(const char* filepath, long* length)
{
  FILE* file = fopen(filepath, "rb");
  if (!file)
  {
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  *length = ftell(file);
  if (*length <= 0)
  {
    return NULL;
    fclose(file);
  }

  fseek(file, 0, SEEK_SET);

  char* content = malloc(*length + 1);
  if (content)
  {
    fread(content, 1, *length, file);
    content[*length] = '\0';
  }

  fclose(file);

  return content;
}

void preprocess_list_file(char* list, long length)
{
  long index = 0;
  while (index < length)
  {
    const char c = list[index];

    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
    {
      // Zero out whitespaces
      list[index] = '\0';
    }
    else if (c == '"')
    {
      // Fast-forward through quotes so that whitespaces persist
      // But zero out the beginning and ending quotes
      list[index] = '\0';
      do
      {
        ++index;
      } while (list[index] != '"' && index < length);

      if (list[index] == '"')
      {
        list[index] = '\0';
      }
    }
    else if (c == '#')
    {
      // Zero out all characters in a comment until a new line occurs
      do
      {
        list[index] = '\0';
        ++index;
      } while (list[index] != '\n' && list[index] != '\r' && index < length);
      continue;
    }

    ++index;
  }
}