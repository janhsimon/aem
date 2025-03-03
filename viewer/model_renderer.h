#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>

bool load_model_renderer();
void destroy_model_renderer();

GLuint get_fallback_diffuse_texture();
GLuint get_fallback_normal_texture();
GLuint get_fallback_orm_texture();

void fill_model_renderer_buffers(GLsizeiptr model_vertex_buffer_size,
                                 const void* model_vertex_buffer,
                                 GLsizeiptr model_index_buffer_size,
                                 const void* model_index_buffer,
                                 uint32_t joint_count);

void prepare_model_draw(const vec3 light_dir, const vec3 camera_pos, mat4 world_matrix, mat4 viewproj_matrix);

void set_material_uniforms(mat3 base_color_uv_transform, mat3 normal_uv_transform, mat3 orm_uv_transform);