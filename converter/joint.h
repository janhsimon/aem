#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_node cgltf_node;

void setup_joint_output(const cgltf_data* input_file);

int32_t calculate_aem_joint_index_from_glb_joint(const cgltf_node* joint);

uint32_t get_joint_count();
uint32_t calculate_keyframe_count(const cgltf_data* input_file);

void write_joints(FILE* output_file);

void write_sequences(const cgltf_data* input_file, FILE* output_file);
void write_keyframes(const cgltf_data* input_file, FILE* output_file);

void destroy_joint_output(const cgltf_data* input_file);