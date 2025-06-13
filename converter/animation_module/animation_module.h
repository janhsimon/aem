#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct cgltf_data cgltf_data;
typedef struct cgltf_node cgltf_node;

void anim_create(const cgltf_data* input_file);

uint32_t anim_get_joint_count();
uint32_t anim_get_keyframe_count();

int32_t anim_calculate_joint_index_for_node(const cgltf_node* node);
bool anim_does_joint_exist_for_node(const cgltf_node* node);

void anim_calculate_global_node_transform(cgltf_node* node, mat4 transform);

void anim_write_joints(FILE* output_file);
void anim_write_animations(FILE* output_file);
void anim_write_tracks(FILE* output_file);
void anim_write_keyframes(FILE* output_file);

void anim_free();