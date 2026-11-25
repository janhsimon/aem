#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void write_header(const cgltf_data* input_file, FILE* output_file);