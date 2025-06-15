#include "util.h"

#include <stdio.h>
#include <stdlib.h>

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