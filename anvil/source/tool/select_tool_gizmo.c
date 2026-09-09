#include "select_tool_gizmo.h"

#include "select_tool.h"
#include "viewport/viewport.h"

#include <util/util.h>

#include <cglm/vec2.h>

#include <glad/gl.h>

#include <stdint.h>

static GLuint vertex_array, vertex_buffer;
static GLuint shader_program;
static GLint viewport_size_uniform_location, color_uniform_location;

static vec2 vertices[4]; // Selection corners

bool generate_select_tool_gizmo()
{
  // Generate select tool vertex array
  {
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    // Generate and fill a vertex buffer
    {
      glGenBuffers(1, &vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    }

    // Apply the vertex definition
    {
      // Position
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
    }
  }

  // Generate line shader program
  {
    GLuint vertex_shader, geometry_shader, fragment_shader;
    if (!util_load_shader("shaders/scale_tool_line.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/scale_tool_line.geo.glsl", GL_GEOMETRY_SHADER, &geometry_shader) ||
        !util_load_shader("shaders/scale_tool_line.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, &geometry_shader, &shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(shader_program);

      viewport_size_uniform_location = util_get_uniform_location(shader_program, "viewport_size");
      color_uniform_location = util_get_uniform_location(shader_program, "color");
    }
  }

  return true;
}

void destroy_select_tool_gizmo()
{
  glDeleteProgram(shader_program);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
}

void draw_select_tool_gizmo(const struct Viewport* viewport)
{
  if (select_tool_get_active_viewport() != viewport)
  {
    return;
  }

  struct Camera* camera = viewport_get_camera(viewport);

  vec2 viewport_size;
  {
    uint32_t w, h;
    viewport_get_resolution(viewport, &w, &h);
    glm_vec2_copy((vec2){ (float)w, (float)h }, viewport_size);
  }

  vec2 min, max;
  select_tool_get_rect(min, max);

  {
    vertices[0][0] = min[0];
    vertices[0][1] = min[1];

    vertices[1][0] = max[0];
    vertices[1][1] = min[1];

    vertices[2][0] = max[0];
    vertices[2][1] = max[1];

    vertices[3][0] = min[0];
    vertices[3][1] = max[1];
  }

  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  // Draw dashed lines
  {
    glUseProgram(shader_program);
    glUniform2fv(viewport_size_uniform_location, 1, (float*)viewport_size);
    glUniform3f(color_uniform_location, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
  }
}