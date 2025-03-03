#pragma once

//#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void setup_animation_output(const cgltf_data* input_file);

void write_animations(const cgltf_data* input_file, FILE* output_file);

void destroy_animation_output();