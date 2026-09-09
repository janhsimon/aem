#include "move_tool_gizmo.h"

#include "gui/gui.h"
#include "scale_tool.h"
#include "scene.h"
#include "tool_state.h"
#include "viewport/viewport.h"

#include <util/util.h>

#include <cglm/vec2.h>

#include <glad/gl.h>

#include <stdint.h>

static GLuint vertex_array, vertex_buffer;
static GLuint line_shader_program, point_shader_program;
static GLint viewport_size_uniform_location, color_uniform_location;

static vec2 vertices[12]; // 0-3 = corners, 4-7 = corner control points, 8-11 mid control points

static struct ToolState* tool_state = NULL;

bool generate_move_tool_gizmo(struct ToolState* tool_state_)
{
  tool_state = tool_state_;

  // Generate scale tool vertex array
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

    if (!util_generate_shader_program(vertex_shader, fragment_shader, &geometry_shader, &line_shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(geometry_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(line_shader_program);

      viewport_size_uniform_location = util_get_uniform_location(line_shader_program, "viewport_size");
      color_uniform_location = util_get_uniform_location(line_shader_program, "color");
    }
  }

  // Generate point shader program
  {
    GLuint vertex_shader, fragment_shader;
    if (!util_load_shader("shaders/scale_tool_point.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
        !util_load_shader("shaders/scale_tool_point.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
    {
      return false;
    }

    if (!util_generate_shader_program(vertex_shader, fragment_shader, NULL, &point_shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }

  return true;
}

void destroy_move_tool_gizmo()
{
  glDeleteProgram(line_shader_program);
  glDeleteProgram(point_shader_program);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteVertexArrays(1, &vertex_array);
}

void draw_move_tool_gizmo(const struct Viewport* viewport)
{
  if (!scene_has_selection())
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

  scale_tool_calc_selection_corners(camera, vertices);
  scale_tool_corners_to_control_points_clip(viewport_size, vertices, &vertices[4]);

  glBindVertexArray(vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

  // Draw dashed lines
  {
    glUseProgram(line_shader_program);
    glUniform2fv(viewport_size_uniform_location, 1, (float*)viewport_size);
    glUniform3f(color_uniform_location, 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
  }

  // Draw control points
  {
    glUseProgram(point_shader_program);
    glPointSize(8.0f);

    glDrawArrays(GL_POINTS, 4, tool_state->active_tool == Tool_Scale ? 8 : 4);
  }
}