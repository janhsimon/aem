#pragma once

#include <stdbool.h>

bool load_texture(const char* filepath, unsigned int* texture);
void free_texture(unsigned int texture);