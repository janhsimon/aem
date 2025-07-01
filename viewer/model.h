#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>
#include <stdint.h>

bool load_model(const char* filepath, const char* path);
void finish_loading_model();
void destroy_model();

void* get_model_vertex_buffer();
uint32_t get_model_vertex_count();

void* get_model_index_buffer();
uint32_t get_model_index_count();

uint32_t get_model_joint_count();
struct AEMJoint* get_model_joints();

uint32_t get_model_animation_count();
char** get_model_animation_names();
float get_model_animation_duration(unsigned int animation_index);
uint32_t get_model_joint_translation_keyframe_count(uint32_t animation_index, uint32_t joint_index);
uint32_t get_model_joint_rotation_keyframe_count(uint32_t animation_index, uint32_t joint_index);
uint32_t get_model_joint_scale_keyframe_count(uint32_t animation_index, uint32_t joint_index);
void evaluate_model_animation(int animation_index, float time);

void draw_model_opaque();
void draw_model_transparent();
void draw_model_wireframe();