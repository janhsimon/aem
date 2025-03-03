#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;

void setup_joint_output(const cgltf_data* input_file);

uint32_t get_joint_count();

void write_joints(FILE* output_file);

void destroy_joint_output();