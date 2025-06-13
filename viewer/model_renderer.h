#pragma once

#include "display_state.h"

#include <cglm/types.h>

#include <glad/gl.h>

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  RenderPass_Opaque,
  RenderPass_Transparent
} RenderPass;

bool load_model_renderer();
void destroy_model_renderer();

void fill_model_renderer_buffers(GLsizeiptr model_vertex_buffer_size,
                                 const void* model_vertex_buffer,
                                 GLsizeiptr model_index_buffer_size,
                                 const void* model_index_buffer,
                                 uint32_t joint_count);

void prepare_model_draw(RenderMode render_mode,
                        PostprocessingMode postprocessing_mode,
                        const vec4 ambient_color,
                        const vec3 light_dir,
                        const vec4 light_color,
                        const vec3 camera_pos,
                        mat4 world_matrix,
                        mat4 viewproj_matrix);

void set_pass(RenderPass pass);