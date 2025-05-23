#include "model_renderer.h"

#include "shader.h"
#include "texture.h"

#include <aem/aem.h>

#include <util/util.h>

#include <stdio.h>

static GLuint vertex_array, vertex_buffer, index_buffer, joint_transform_buffer, joint_transform_texture;

static GLuint shader_program;
static GLint light_dir_uniform_location, camera_pos_uniform_location, world_uniform_location, viewproj_uniform_location;
static GLint base_color_uv_transform_uniform_location, normal_uv_transform_uniform_location,
  orm_uv_transform_uniform_location;

static GLuint fallback_diffuse_texture, fallback_normal_texture, fallback_orm_texture;

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

      light_dir_uniform_location = get_uniform_location(shader_program, "light_dir");
      camera_pos_uniform_location = get_uniform_location(shader_program, "camera_pos");

      base_color_uv_transform_uniform_location = get_uniform_location(shader_program, "base_color_uv_transform");
      normal_uv_transform_uniform_location = get_uniform_location(shader_program, "normal_uv_transform");
      orm_uv_transform_uniform_location = get_uniform_location(shader_program, "orm_uv_transform");

      const GLint joint_transform_tex_uniform_location = get_uniform_location(shader_program, "joint_transform_tex");
      glUniform1i(joint_transform_tex_uniform_location, 0);

      const GLint base_color_tex_uniform_location = get_uniform_location(shader_program, "base_color_tex");
      glUniform1i(base_color_tex_uniform_location, 1);

      const GLint normal_tex_uniform_location = get_uniform_location(shader_program, "normal_tex");
      glUniform1i(normal_tex_uniform_location, 2);

      const GLint orm_tex_uniform_location = get_uniform_location(shader_program, "orm_tex");
      glUniform1i(orm_tex_uniform_location, 3);
    }
  }

  glGenTextures(1, &joint_transform_texture);

  fallback_diffuse_texture = load_builtin_texture("textures/fallback_diffuse.png");
  fallback_normal_texture = load_builtin_texture("textures/fallback_normal.png");
  fallback_orm_texture = load_builtin_texture("textures/fallback_orm.png");

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

  glDeleteTextures(1, &fallback_diffuse_texture);
  glDeleteTextures(1, &fallback_normal_texture);
  glDeleteTextures(1, &fallback_orm_texture);
}

GLuint get_fallback_diffuse_texture()
{
  return fallback_diffuse_texture;
}

GLuint get_fallback_normal_texture()
{
  return fallback_normal_texture;
}

GLuint get_fallback_orm_texture()
{
  return fallback_orm_texture;
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

void prepare_model_draw(const vec3 light_dir, const vec3 camera_pos, mat4 world_matrix, mat4 viewproj_matrix)
{
  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  // Set light direction uniform
  glUniform3fv(light_dir_uniform_location, 1, light_dir);

  // Set camera position uniform
  glUniform3fv(camera_pos_uniform_location, 1, camera_pos);

  // Set world matrix uniform
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, joint_transform_texture);
}

void set_material_uniforms(mat3 base_color_uv_transform, mat3 normal_uv_transform, mat3 orm_uv_transform)
{
  glUniformMatrix3fv(base_color_uv_transform_uniform_location, 1, GL_FALSE, (float*)base_color_uv_transform);
  glUniformMatrix3fv(normal_uv_transform_uniform_location, 1, GL_FALSE, (float*)normal_uv_transform);
  glUniformMatrix3fv(orm_uv_transform_uniform_location, 1, GL_FALSE, (float*)orm_uv_transform);
}