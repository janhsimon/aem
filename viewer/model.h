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

struct AEMAnimationChannel* get_model_animation_channel(uint32_t channel_index);

void blend_to_model_animation_channel(uint32_t channel_index);

bool get_model_animation_mixer_enabled();
void set_model_animation_mixer_enabled(bool enabled);

float get_model_animation_mixer_blend_speed();
void set_model_animation_mixer_blend_speed(float blend_speed);

enum AEMAnimationBlendMode get_model_animation_mixer_blend_mode();
void set_model_animation_mixer_blend_mode(enum AEMAnimationBlendMode blend_mode);

uint32_t get_model_animation_count();
char** get_model_animation_names();

float get_model_animation_duration(unsigned int animation_index);

uint32_t get_model_joint_translation_keyframe_count(uint32_t animation_index, uint32_t joint_index);
uint32_t get_model_joint_rotation_keyframe_count(uint32_t animation_index, uint32_t joint_index);
uint32_t get_model_joint_scale_keyframe_count(uint32_t animation_index, uint32_t joint_index);

void model_update_animation(float delta_time);

void draw_model_opaque();
void draw_model_transparent();
void draw_model_wireframe();