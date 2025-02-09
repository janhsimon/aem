#pragma once

#include <stdint.h>
#include <stdio.h>

struct cgltf_data;

void write_header(const struct cgltf_data* input_file, FILE* output_file);