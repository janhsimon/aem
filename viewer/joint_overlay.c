#include "joint_overlay.h"

#include <util/util.h>

#include <glad/gl.h>

#define VERTEX_SIZE 12 // The size of a joint overlay vertex in bytes

static const vec4 color_unselected = { 0.0f, 0.0f, 1.0f, 1.0f };
static const vec4 color_selected = { 1.0f, 0.0f, 0.0f, 1.0f };

static GLuint vertex_array, vertex_buffer;
static GLuint shader_program;
static GLint world_uniform_location, viewproj_uniform_location;
static GLint color_uniform_location;

bool generate_joint_overlay()
{
  const float vertices[3] = { 0.0f, 0.0f, 0.0f }; // TODO: This is not necessary, can be done in vertex shader

  // Generate bone overlay vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(VERTEX_SIZE), vertices, GL_STATIC_DRAW);
    }

    // Apply the vertex definition
    {
      // Position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE, (void*)0);
    }
  }

  // Generate shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!load_shader("shaders/bone_overlay.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !load_shader("shaders/overlay.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, NULL, &shader_program))
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

void destroy_joint_overlay()
{
  glDeleteProgram(shader_program);

  glDeleteBuffers(1, &vertex_buffer);

  glDeleteVertexArrays(1, &vertex_array);
}

void draw_joint_overlay(mat4 world_matrix, mat4 viewproj_matrix, bool selected)
{
  glPointSize(10.0f);

  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vertex_array);
  glUseProgram(shader_program);

  // Set world matrix uniform
  glUniformMatrix4fv(world_uniform_location, 1, GL_FALSE, (float*)world_matrix);

  // Set view-projection matrix uniform
  glUniformMatrix4fv(viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);

  // Set color uniform
  glUniform4fv(color_uniform_location, 1, selected ? color_selected : color_unselected);

  // Draw the joint overlay
  glDrawArrays(GL_POINTS, 0, 1);

  glEnable(GL_DEPTH_TEST);
}
