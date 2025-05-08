#include "bone_overlay.h"

#include "shader.h"

static const vec4 color = { 0.8f, 0.8f, 0.8f, 1.0f };

static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location;
static GLint color_uniform_location;

bool generate_wireframe_overlay()
{
  // Generate shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/wireframe_overlay.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !load_shader("shaders/overlay.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      world_uniform_location = get_uniform_location(shader_program, "world");
      viewproj_uniform_location = get_uniform_location(shader_program, "viewproj");

      color_uniform_location = get_uniform_location(shader_program, "color");
    }
  }

  return true;
}

void destroy_wireframe_overlay()
{
  glDeleteProgram(shader_program);
}

void begin_draw_wireframe_overlay(mat4 world_matrix, mat4 viewproj_matrix)
{
  glUseProgram(shader_program);

  // Set world matrix uniform
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  // Set color uniform
  glUniform4fv(color_uniform_location, 1, color);
}

// void draw_wireframe_overlay(mat4 world_matrix, mat4 viewproj_matrix, bool selected)
//{
//   glDisable(GL_DEPTH_TEST);
//
//   glBindVertexArray(vertex_array);
//   glUseProgram(shader_program);
//
//   // Set world matrix uniform
//   glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
//
//   // Set view-projection matrix uniform
//   glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
//
//   // Set color uniform
//   glUniform4fv(color_uniform_location, 1, selected ? color_selected : color_unselected);
//
//   // Draw the bone overlay
//   glDrawElements(GL_LINE_LOOP, INDEX_COUNT, GL_UNSIGNED_INT, NULL);
//
//   glEnable(GL_DEPTH_TEST);
// }
