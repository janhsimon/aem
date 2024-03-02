#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>

bool load_model_renderer();
void destroy_model_renderer();

void fill_model_renderer_buffers(GLsizeiptr vertices_size,
                                 const struct Vertex* vertices,
                                 GLsizeiptr indices_size,
                                 const void* indices,
                                 uint32_t bone_count);

void prepare_model_draw(const vec3 light_dir, const vec3 camera_pos, mat4 world_matrix, mat4 viewproj_matrix);