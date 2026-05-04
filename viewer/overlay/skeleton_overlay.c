#include "skeleton_overlay.h"

#include "model.h"
#include "skeleton_state.h"
#include "skeleton_tool.h"

#include <util/util.h>

#include <glad/gl.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimguizmo.h>

static GLuint vertex_array, vertex_buffer;
static GLuint skeleton_shader_program, selected_joint_shader_program;
static GLint skeleton_viewproj_uniform_location, skeleton_screen_resolution_uniform_location;
static GLint selected_joint_viewproj_uniform_location, selected_joint_screen_resolution_uniform_location;

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

      skeleton_viewproj_uniform_location = get_uniform_location(skeleton_shader_program, "viewproj");
      skeleton_screen_resolution_uniform_location = get_uniform_location(skeleton_shader_program, "screen_resolution");

      glUseProgram(selected_joint_shader_program);

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
}

void draw_skeleton_overlay(struct SkeletonState* skeleton_state,
                           mat4 world_matrix,
                           mat4 view_matrix,
                           mat4 proj_matrix,
                           vec2 screen_resolution)
{
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, skeleton_tool_get_points_size(), skeleton_tool_get_points(), GL_STATIC_DRAW);

  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vertex_array);

  mat4 viewproj_matrix;
  glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);

  // Draw the skeleton with the joints and lines
  {
    glUseProgram(skeleton_shader_program);
    glUniformMatrix4fv(skeleton_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
    glUniform2fv(skeleton_screen_resolution_uniform_location, 1, (float*)screen_resolution);

    glDrawArrays(GL_LINES, 0, skeleton_tool_get_point_count());
  }

  // Now draw the hovered joint
  if (skeleton_state->hover_joint_index >= 0)
  {
    glUseProgram(selected_joint_shader_program);
    glUniformMatrix4fv(selected_joint_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
    glUniform2fv(selected_joint_screen_resolution_uniform_location, 1, (float*)screen_resolution);

    int32_t hovered_point_index = skeleton_tool_get_hovered_point_index(skeleton_state);
    if (hovered_point_index >= 0)
    {
      glDrawArrays(GL_POINTS, hovered_point_index, 1);
    }
  }

  glEnable(GL_DEPTH_TEST);

  // Move tool
  {
    if (skeleton_state->selected_joint_index < 0)
    {
      return;
    }

    // 1. Get model-space transforms
    mat4 base_model, combined_model;
    get_model_animation_joint_base_transform(skeleton_state->selected_joint_index, base_model);
    get_model_animation_joint_combined_transform(skeleton_state->selected_joint_index, combined_model);

    // 2. Convert to world for gizmo
    mat4 combined_world;
    glm_mat4_mul(world_matrix, combined_model, combined_world);

    // 3. Manipulate
    {
      OPERATION operation = 0;
      {
        if (skeleton_state->translate_enabled)
        {
          operation |= TRANSLATE;
        }

        if (skeleton_state->rotate_enabled)
        {
          operation |= ROTATE;
        }

        if (skeleton_state->scale_enabled)
        {
          operation |= SCALEU;
        }
      }

      const MODE mode = (skeleton_state->tool_move_mode == SkeletonToolMoveMode_Global) ? WORLD : LOCAL;

      ImGuizmo_Manipulate((float*)view_matrix, (float*)proj_matrix, operation, mode, (float*)combined_world, NULL, NULL,
                          NULL, NULL);
    }

    // 4. Back to model space
    mat4 inv_world, new_model;
    glm_mat4_inv(world_matrix, inv_world);
    glm_mat4_mul(inv_world, combined_world, new_model);

    // 5. Compute delta
    mat4 inv_anim, delta_model;
    glm_mat4_inv(base_model, inv_anim);
    glm_mat4_mul(inv_anim, new_model, delta_model);

    // 6. Store
    set_model_animation_joint_additive_transform(skeleton_state->selected_joint_index, delta_model);
  }
}