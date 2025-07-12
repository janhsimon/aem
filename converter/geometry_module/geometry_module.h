#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_primitive cgltf_primitive;

bool geo_is_primitive_valid(const cgltf_primitive* primitive);

void geo_create(const cgltf_data* input_file);

uint32_t geo_calculate_vertex_count();
uint32_t geo_calculate_index_count();

uint32_t geo_get_mesh_count();

void geo_write_vertex_buffer(FILE* output_file);
void geo_write_index_buffer(FILE* output_file);
void geo_write_meshes(FILE* output_file);

void geo_free();