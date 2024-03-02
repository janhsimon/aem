#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>
#include <stdint.h>

bool load_model(const char* filepath, const char* path);
void destroy_model();

struct Vertex* get_model_vertices();
uint32_t get_model_vertices_size();

void* get_model_indices();
uint32_t get_model_indices_size();

uint32_t get_model_bone_count();

uint32_t get_model_animation_count();
char** get_model_animation_names();
float get_model_animation_duration(unsigned int animation_index);
void evaluate_model_animation(int animation_index, float time);

void draw_model();
void draw_model_bone_overlay(bool bind_pose, mat4 world_matrix, mat4 viewproj_matrix);