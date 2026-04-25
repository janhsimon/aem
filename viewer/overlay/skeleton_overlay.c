#include "skeleton_overlay.h"

#include "input.h"
#include "model.h"
#include "skeleton_state.h"

#include <util/util.h>

#include <aem/model.h>

#include <cglm/affine.h>
#include <cglm/vec2.h>
#include <cglm/vec3.h>

#include <glad/gl.h>

#include <assert.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimguizmo.h>

static GLuint vertex_array, vertex_buffer;
static GLuint skeleton_shader_program, selected_joint_shader_program;
static GLint skeleton_viewproj_uniform_location, skeleton_screen_resolution_uniform_location;
static GLint selected_joint_viewproj_uniform_location, selected_joint_screen_resolution_uniform_location;

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

  free(points);
}

void skeleton_overlay_on_new_model_loaded()
{
  struct AEMJoint* joints = get_model_joints();
  const uint32_t joint_count = get_model_joint_count();

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

    points[point_index++].joint_index = joint->parent_joint_index;
    points[point_index++].joint_index = joint_index;
  }
}

void draw_skeleton_overlay(struct SkeletonState* skeleton_state,
                           mat4 world_matrix,
                           mat4 view_matrix,
                           mat4 proj_matrix,
                           vec2 screen_resolution)
{
  for (uint32_t point_index = 0; point_index < point_count; ++point_index)
  {
    Point* point = &points[point_index];

    mat4 combined_model;
    get_model_animation_joint_combined_transform(point->joint_index, combined_model);

    mat4 combined_world;
    glm_mat4_mul(world_matrix, combined_model, combined_world);

    glm_mat4_mulv3(combined_world, GLM_VEC3_ZERO, 1.0f, point->position);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Point) * point_count, points, GL_STATIC_DRAW);

  glDisable(GL_DEPTH_TEST);

  glBindVertexArray(vertex_array);

  mat4 viewproj_matrix;
  glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);

  // Picking logic
  {
    uint32_t select_candidate;
    float min_distance = -1.0f;
    for (uint32_t point_index = 0; point_index < point_count; ++point_index)
    {
      Point* point = &points[point_index];

      vec4 world;
      glm_vec3_copy(point->position, world);
      world[3] = 1.0f;

      vec4 clip;
      glm_mat4_mulv(viewproj_matrix, world, clip);

      if (clip[3] > 0.0f)
      {
        glm_vec2_divs(clip, clip[3], clip);
        clip[1] = -clip[1];

        vec2 mouse_pos;
        get_mouse_pos(mouse_pos);

        glm_vec2_div(mouse_pos, screen_resolution, mouse_pos);
        glm_vec2_scale(mouse_pos, 2.0f, mouse_pos);
        glm_vec2_subs(mouse_pos, 1.0f, mouse_pos);

        const float distance = glm_vec2_distance(clip, mouse_pos);
        if (distance < min_distance || min_distance < 0.0f)
        {
          select_candidate = point_index;
          min_distance = distance;
        }
      }
    }

    if (min_distance >= 0.0f && min_distance < 0.015f)
    {
      skeleton_state->hover_joint_index = points[select_candidate].joint_index;
    }
  }

  // Draw the skeleton with the joints and lines
  {
    glUseProgram(skeleton_shader_program);
    glUniformMatrix4fv(skeleton_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
    glUniform2fv(skeleton_screen_resolution_uniform_location, 1, (float*)screen_resolution);

    glDrawArrays(GL_LINES, 0, point_count);
  }

  // Now draw the hovered joint
  if (skeleton_state->hover_joint_index >= 0)
  {
    glUseProgram(selected_joint_shader_program);
    glUniformMatrix4fv(selected_joint_viewproj_uniform_location, 1, GL_FALSE, (float*)viewproj_matrix);
    glUniform2fv(selected_joint_screen_resolution_uniform_location, 1, (float*)screen_resolution);

    for (uint32_t point_index = 0; point_index < point_count; ++point_index)
    {
      const Point* point = &points[point_index];
      if (point->joint_index == skeleton_state->hover_joint_index)
      {
        glDrawArrays(GL_POINTS, point_index, 1);
        break;
      }
    }
  }

  glEnable(GL_DEPTH_TEST);

  // Guizmo test
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
    ImGuizmo_Manipulate((float*)view_matrix, (float*)proj_matrix, UNIVERSAL, WORLD, (float*)combined_world, NULL, NULL,
                        NULL, NULL);

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