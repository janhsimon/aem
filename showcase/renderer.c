#include "renderer.h"

#include "camera.h"
#include "model_manager.h"

#include <aem/model.h>

#include <util/util.h>

#include <cglm/mat4.h>
#include <glad/gl.h>

#define CLEAR_COLOR 0.58f, 0.71f, 1.0f

static GLuint vertex_array, shader_program;
static GLint world_uniform_location, viewproj_uniform_location, render_pass_uniform_location,
  camera_pos_uniform_location;

static mat4 view_matrix;

bool load_renderer()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  bind_vertex_index_buffers();

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

  // Load shaders
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/vert.glsl", GL_VERTEX_SHADER, &vertex_shader))
    {
      return false;
    }

    if (!load_shader("shaders/frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
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
      render_pass_uniform_location = get_uniform_location(shader_program, "render_pass");
      camera_pos_uniform_location = get_uniform_location(shader_program, "camera_pos");

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

  return true;
}

void clear_frame()
{

  glClearColor(CLEAR_COLOR, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void start_render_frame()
{
  glBindVertexArray(vertex_array);

  glUseProgram(shader_program);

  // Set the latest camera position
  {
    vec3 camera_position;
    cam_get_position(camera_position);
    glUniform3fv(camera_pos_uniform_location, 1, camera_position);
  }

  calc_view_matrix(view_matrix);
}

void use_fov(float aspect, float fov)
{
  mat4 proj_matrix;
  calc_proj_matrix(aspect, fov, proj_matrix);

  glm_mat4_mul(proj_matrix, view_matrix, proj_matrix);
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)proj_matrix);
}

void use_world_matrix(mat4 world_matrix)
{
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
}

void use_render_pass(enum RenderPass pass)
{
  glUniform1i(render_pass_uniform_location, (GLint)pass);
}

void free_renderer()
{
  glDeleteVertexArrays(1, &vertex_array);
  glDeleteProgram(shader_program);
}