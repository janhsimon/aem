#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>
#include <stdint.h>

bool load_model(const char* filepath, const char* path);
void finish_loading_model();
void destroy_model();

void* get_model_vertex_buffer();
uint64_t get_model_vertex_buffer_size();

void* get_model_index_buffer();
uint64_t get_model_index_buffer_size();

void* get_model_image_buffer();
uint64_t get_model_image_buffer_size();

uint32_t get_model_joint_count();
const struct AEMJoint* get_model_joints();

uint32_t get_model_animation_count();
char** get_model_animation_names();
float get_model_animation_duration(unsigned int animation_index);
void evaluate_model_animation(int animation_index, float time);

void draw_model();
void draw_model_wireframe_overlay();
void draw_model_joint_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix, int32_t selected_bone_index);
void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix, int32_t selected_bone_index);