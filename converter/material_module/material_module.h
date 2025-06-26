#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_material cgltf_material;

void mat_create(const cgltf_data* input_file, const char* path);

void mat_free();

bool mat_get_texture_transform_for_material(const cgltf_material* material, mat3 transform);

uint32_t mat_calculate_image_buffer_size();
uint32_t mat_get_texture_count();
uint32_t mat_get_material_count();

void mat_write_image_buffer(FILE* output_file);
void mat_write_textures(FILE* output_file);
void mat_write_materials(FILE* output_file);
