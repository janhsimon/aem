#pragma once

#include <stdio.h>

struct cgltf_data;

void write_materials(const struct cgltf_data* input_file, FILE* output_file);
