#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void setup_texture_output(const cgltf_data* input_file);

uint64_t calculate_image_buffer_size();
uint32_t calculate_level_count();

void write_image_buffer(const char* path, FILE* output_file);

void write_levels(FILE* output_file);
void write_textures(FILE* output_file);

void destroy_texture_output();