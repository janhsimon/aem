#pragma once

#include <cglm/mat4.h>

#include <glad/gl.h>

#include <stdbool.h>

bool load_model_renderer();
void destroy_model_renderer();

GLint get_bone_transforms_uniform_location();

void fill_model_renderer_buffers(GLsizeiptr vertices_size,
                                 const struct Vertex* vertices,
                                 GLsizeiptr indices_size,
                                 const void* indices);

void prepare_model_draw(const vec3 light_dir, const vec3 camera_pos, mat4 world_matrix, mat4 viewproj_matrix);