#pragma once

#include <cglm/mat4.h>

void load_model(const char* filepath, const char* path);
void destroy_model();

uint32_t get_model_animation_count();
char** get_model_animation_names();
float get_model_animation_duration(unsigned int animation_index);
void evaluate_model_animation(int animation_index, float time);

void draw_model(const vec3 light_dir, const vec3 camera_pos, mat4 world_matrix, mat4 viewproj_matrix);
void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix);