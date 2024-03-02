#include "model_renderer.h"

#include "shader.h"

#include <util/util.h>

static GLuint vertex_array, vertex_buffer, index_buffer, uniform_buffer;

static GLuint shader_program;
static GLint light_dir_uniform_location, camera_pos_uniform_location, world_uniform_location, viewproj_uniform_location;

bool load_model_renderer()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &index_buffer);
  glGenBuffers(1, &uniform_buffer);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

  // Apply the vertex definition
  {
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(0 * 4));

    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(3 * 4));

    // Tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(6 * 4));

    // Bitangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(9 * 4));

    // UV
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(12 * 4));

    // Bone indices
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, VERTEX_SIZE, (void*)(14 * 4));

    // Bone weights
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)(18 * 4));

    // Extra bone index
    glEnableVertexAttribArray(7);
    glVertexAttribIPointer(7, 1, GL_INT, VERTEX_SIZE, (void*)(22 * 4));
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

      const GLint base_color_tex_uniform_location = get_uniform_location(shader_program, "base_color_tex");
      glUniform1i(base_color_tex_uniform_location, 0);

      const GLint normal_tex_uniform_location = get_uniform_location(shader_program, "normal_tex");
      glUniform1i(normal_tex_uniform_location, 1);

      const GLint orm_tex_uniform_location = get_uniform_location(shader_program, "orm_tex");
      glUniform1i(orm_tex_uniform_location, 2);
    }
  }

  return true;
}

void destroy_model_renderer()
{
  glDeleteProgram(shader_program);

  glDeleteBuffers(1, &uniform_buffer);
  glDeleteBuffers(1, &index_buffer);
  glDeleteBuffers(1, &vertex_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void fill_model_renderer_buffers(GLsizeiptr vertices_size,
                                 const struct Vertex* vertices,
                                 GLsizeiptr indices_size,
                                 const void* indices,
                                 uint32_t bone_count)
{
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertices_size, (const void*)vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);

  glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_buffer, 0, sizeof(mat4) * bone_count);
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
}