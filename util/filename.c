#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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