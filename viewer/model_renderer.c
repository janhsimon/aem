#include "model_renderer.h"

#include "texture.h"

#include <aem/aem.h>

#include <util/util.h>

#include <stdio.h>

static GLuint vertex_array, vertex_buffer, index_buffer, joint_transform_buffer, joint_transform_texture;

static GLuint shader_program;
static GLint pass_uniform_location, ambient_color_uniform_location, light_dir_uniform_location, light_color_uniform_location,
  camera_pos_uniform_location, render_mode_uniform_location, postprocessing_mode_uniform_location,
  world_uniform_location, viewproj_uniform_location;

bool load_model_renderer()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &index_buffer);
  glGenBuffers(1, &joint_transform_buffer);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

  // Apply the vertex definition
  {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(0 * 4));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(3 * 4));

    // Tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(6 * 4));

    // Bitangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(9 * 4));

    // UV
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(12 * 4));

    // Joint indices
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, AEM_VERTEX_SIZE, (void*)(14 * 4));

    // Joint weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, AEM_VERTEX_SIZE, (void*)(18 * 4));
  }

  // Generate shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/model.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !load_shader("shaders/model.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations and set constant uniforms
    {
      glUseProgram(shader_program);

      world_uniform_location = get_uniform_location(shader_program, "world");
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

      pass_uniform_location = get_uniform_location(shader_program, "render_pass");
      ambient_color_uniform_location = get_uniform_location(shader_program, "ambient_color");
      light_dir_uniform_location = get_uniform_location(shader_program, "light_dir");
      light_color_uniform_location = get_uniform_location(shader_program, "light_color");
      camera_pos_uniform_location = get_uniform_location(shader_program, "camera_pos");
      render_mode_uniform_location = get_uniform_location(shader_program, "render_mode");
      postprocessing_mode_uniform_location = get_uniform_location(shader_program, "postprocessing_mode");

      const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
      glUniform1i(joint_transform_tex_uniform_location, 0);

      const GLint base_color_tex_uniform_location = get_uniform_location(shader_program, "base_color_tex");
      glUniform1i(base_color_tex_uniform_location, 1);

      const GLint normal_tex_uniform_location = get_uniform_location(shader_program, "normal_tex");
      glUniform1i(normal_tex_uniform_location, 2);

      const GLint pbr_tex_uniform_location = get_uniform_location(shader_program, "pbr_tex");
      glUniform1i(pbr_tex_uniform_location, 3);
    }
  }

  glGenTextures(1, &joint_transform_texture);

  return true;
}

void destroy_model_renderer()
{
  glDeleteProgram(shader_program);

  glDeleteTextures(1, &joint_transform_texture);

  glDeleteBuffers(1, &joint_transform_buffer);
  glDeleteBuffers(1, &index_buffer);
  glDeleteBuffers(1, &vertex_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void fill_model_renderer_buffers(GLsizeiptr model_vertex_buffer_size,
                                 const void* model_vertex_buffer,
                                 GLsizeiptr model_index_buffer_size,
                                 const void* model_index_buffer,
                                 uint32_t joint_count)
{
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, model_vertex_buffer_size, model_vertex_buffer, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model_index_buffer_size, model_index_buffer, GL_STATIC_DRAW);

  glBindBuffer(GL_TEXTURE_BUFFER, joint_transform_buffer);
  glBufferData(GL_TEXTURE_BUFFER, sizeof(mat4) * joint_count, NULL, GL_DYNAMIC_DRAW);

  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, joint_transform_buffer);
}

void prepare_model_draw(RenderMode render_mode,
                        PostprocessingMode postprocessing_mode,
                        const vec4 ambient_color,
                        const vec3 light_dir,
                        const vec4 light_color,
                        const vec3 camera_pos,
                        mat4 world_matrix,
                        mat4 viewproj_matrix)
{
  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  // Set uniforms
  glUniform4fv(ambient_color_uniform_location, 1, ambient_color);
  glUniform3fv(light_dir_uniform_location, 1, light_dir);
  glUniform4fv(light_color_uniform_location, 1, light_color);
  glUniform3fv(camera_pos_uniform_location, 1, camera_pos);
  glUniform1i(render_mode_uniform_location, (GLint)render_mode);
  glUniform1i(postprocessing_mode_uniform_location, (GLint)postprocessing_mode);
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
}

void set_pass(RenderPass pass)
{
  glUniform1i(pass_uniform_location, (GLint)pass);
}