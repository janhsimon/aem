#pragma once

#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void write_materials(const cgltf_data* input_file, FILE* output_file);
