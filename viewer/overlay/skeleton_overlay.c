#include "skeleton_overlay.h"

#include <util/util.h>

#include <cglm/vec3.h>

#include <glad/gl.h>

#include <assert.h>

static GLuint vertex_array, vertex_buffer;
static GLuint skeleton_shader_program, selected_joint_shader_program;
static GLint skeleton_world_uniform_location, skeleton_viewproj_uniform_location,
  skeleton_screen_resolution_uniform_location;
static GLint selected_joint_world_uniform_location, selected_joint_viewproj_uniform_location,
  selected_joint_screen_resolution_uniform_location;

struct
{
  vec3 position;
  int32_t joint_index;
} typedef Point;

static Point* points;
static uint32_t point_count = 0;

bool generate_skeleton_overlay()
{
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 16, (void*)0);

  glEnableVertexAttribArray(1);
  glVertexAttribIPointer(1, 1, GL_INT, 16, (void*)12);

  GLuint vertex_shader, skeleton_geometry_shader, selected_joint_geometry_shader, fragment_shader;
  if (!load_shader("shaders/overlay/skeleton.vert.glsl", GL_VERTEX_SHADER, &vertex_shader) ||
      !load_shader("shaders/overlay/skeleton_line.geo.glsl", GL_GEOMETRY_SHADER, &skeleton_geometry_shader) ||
      !load_shader("shaders/overlay/skeleton_point.geo.glsl", GL_GEOMETRY_SHADER, &selected_joint_geometry_shader) ||
      !load_shader("shaders/overlay/overlay.frag.glsl", GL_FRAGMENT_SHADER, &fragment_shader))
  {
    return false;
  }

  // Generate skeleton shader programs
  {
    if (!generate_shader_program(vertex_shader, fragment_shader, &skeleton_geometry_shader, &skeleton_shader_program))
    {
      return false;
    }

    if (!generate_shader_program(vertex_shader, fragment_shader, &selected_joint_geometry_shader,
                                 &selected_joint_shader_program))
    {
      return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(skeleton_geometry_shader);
    glDeleteShader(selected_joint_geometry_shader);
    glDeleteShader(fragment_shader);

    // Retrieve uniform locations
    {
      glUseProgram(skeleton_shader_program);

      skeleton_world_uniform_location = get_uniform_location(skeleton_shader_program, "world");
      skeleton_viewproj_uniform_location = get_uniform_location(skeleton_shader_program, "viewproj");
      skeleton_screen_resolution_uniform_location = get_uniform_location(skeleton_shader_program, "screen_resolution");
      // render_mode_uniform_location = get_uniform_location(shader_program, "render_mode");
      // selected_uniform_location = get_uniform_location(shader_program, "selected");

      glUseProgram(selected_joint_shader_program);

      selected_joint_world_uniform_location = get_uniform_location(selected_joint_shader_program, "world");
      selected_joint_viewproj_uniform_location = get_uniform_location(selected_joint_shader_program, "viewproj");
      selected_joint_screen_resolution_uniform_location =
        get_uniform_location(selected_joint_shader_program, "screen_resolution");
    }
  }

  return true;
}

void destroy_skeleton_overlay()
{
  glDeleteProgram(skeleton_shader_program);
  glDeleteProgram(selected_joint_shader_program);

  free(points);
}

void skeleton_overlay_on_new_model_loaded(struct AEMJoint* joints, uint32_t joint_count)
{
  // Count the number of bones (connections between joints)
  point_count = 0;
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    const struct AEMJoint* joint = &joints[joint_index];
    if (joint->parent_joint_index >= 0)
    {
      point_count += 2;
    }
  }

  // Allocate the right number of points
  const uint32_t size = sizeof(Point) * point_count;
  points = malloc(size);
  assert(points);

  // Fill the points with the data from the joints (selected joints come last)
  uint32_t point_index = 0;
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    struct AEMJoint* joint = &joints[joint_index];
    if (joint->parent_joint_index < 0)
    {
      continue;
    }

    struct AEMJoint* parent_joint = &joints[joint->parent_joint_index];

    {
      mat4 bind_matrix;
      glm_mat4_make(parent_joint->inverse_bind_matrix, bind_matrix);
      glm_mat4_inv(bind_matrix, bind_matrix);
      glm_mat4_mulv3(bind_matrix, GLM_VEC3_ZERO, 1.0f, points[point_index].position);

      points[point_index].joint_index = joint->parent_joint_index;
      ++point_index;
    }

    {
      mat4 bind_matrix;
      glm_mat4_make(joint->inverse_bind_matrix, bind_matrix);
      glm_mat4_inv(bind_matrix, bind_matrix);
      glm_mat4_mulv3(bind_matrix, GLM_VEC3_ZERO, 1.0f, points[point_index].position);

      points[point_index].joint_index = joint_index;
      ++point_index;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, size, points, GL_STATIC_DRAW);
}

void draw_skeleton_overlay(mat4 world_matrix,
                           mat4 viewproj_matrix,
                           vec2 screen_resolution,
                           int32_t selected_joint_index)
{
  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vertex_array);

  glUseProgram(skeleton_shader_program);
  glUniformMatrix4fv(skeleton_world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
  glUniformMatrix4fv(skeleton_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
  glUniform2fv(skeleton_screen_resolution_uniform_location, 1, (float*)screen_resolution);

  // glUniform1i(render_mode_uniform_location, 0);
  glDrawArrays(GL_LINES, 0, point_count);

  // Now draw the selected joint
  if (selected_joint_index >= 0)
  {
    glUseProgram(selected_joint_shader_program);
    glUniformMatrix4fv(selected_joint_world_uniform_location, 1, GL_FALSE, (float*)world_matrix);
    glUniformMatrix4fv(selected_joint_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
    glUniform2fv(selected_joint_screen_resolution_uniform_location, 1, (float*)screen_resolution);

    for (uint32_t point_index = 0; point_index < point_count; ++point_index)
    {
      const Point* point = &points[point_index];
      if (point->joint_index == selected_joint_index)
      {
        glDrawArrays(GL_POINTS, point_index, 1);
        break;
      }
    }
  }

  glEnable(GL_DEPTH_TEST);
}