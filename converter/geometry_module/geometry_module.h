#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void geo_create(const cgltf_data* input_file);

uint32_t geo_calculate_vertex_count();
uint32_t geo_calculate_index_count();

uint32_t geo_get_mesh_count();

void geo_write_vertex_buffer(FILE* output_file);
void geo_write_index_buffer(FILE* output_file);
void geo_write_meshes(FILE* output_file);

void geo_free();