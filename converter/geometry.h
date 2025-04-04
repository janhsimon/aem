#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void setup_geometry_output(const cgltf_data* input_file);

uint32_t get_mesh_count();

uint64_t calculate_vertex_buffer_size();
uint64_t calculate_index_buffer_size();

void write_vertex_buffer(FILE* output_file);
void write_index_buffer(FILE* output_file);

void write_meshes(FILE* output_file);

void destroy_geometry_output();