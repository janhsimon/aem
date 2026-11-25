#include "skeleton_tool.h"

#include "input.h"
#include "model.h"
#include "skeleton_state.h"

#include <aem/model.h>

#include <cglm/mat4.h>
#include <cglm/vec2.h>

#include <assert.h>

struct
{
  vec3 position;
  int32_t joint_index;
} typedef Point;

static Point* points = NULL;
static uint32_t point_count = 0;

static struct SkeletonState* skeleton_state;

void init_skeleton_tool(struct SkeletonState* skeleton_state_)
{
  skeleton_state = skeleton_state_;
}

void destroy_skeleton_tool()
{
  free(points);
}

void skeleton_tool_on_new_model_loaded()
{
  struct AEMJoint* joints = get_model_joints();
  const uint32_t joint_count = get_model_joint_count();

  skeleton_state->tool_available = (joint_count > 0);
  if (joint_count == 0)
  {
    skeleton_state->tool_active = false;
    return;
  }

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

  // Free up previous points memory
  if (points)
  {
    free(points);
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

void skeleton_tool_reset_selected_joint()
{
  if (skeleton_state->selected_joint_index >= 0)
  {
    set_model_animation_joint_additive_transform(skeleton_state->selected_joint_index, GLM_MAT4_IDENTITY);
  }
}

void skeleton_tool_reset_all_joints()
{
  uint32_t joint_count = get_model_joint_count();
  for (uint32_t joint_index = 0; joint_index < joint_count; ++joint_index)
  {
    set_model_animation_joint_additive_transform(joint_index, GLM_MAT4_IDENTITY);
  }
}

void update_skeleton_tool(mat4 world_matrix, mat4 viewproj_matrix, vec2 screen_resolution)
{
  skeleton_state->hover_joint_index = -1;

  for (uint32_t point_index = 0; point_index < point_count; ++point_index)
  {
    Point* point = &points[point_index];

    mat4 combined_model;
    get_model_animation_joint_combined_transform(point->joint_index, combined_model);

    mat4 combined_world;
    glm_mat4_mul(world_matrix, combined_model, combined_world);

    glm_mat4_mulv3(combined_world, GLM_VEC3_ZERO, 1.0f, point->position);
  }

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
}

uint32_t skeleton_tool_get_points_size()
{
  return sizeof(Point) * point_count;
}

uint32_t skeleton_tool_get_point_count()
{
  return point_count;
}

const void* skeleton_tool_get_points()
{
  return points;
}

int32_t skeleton_tool_get_hovered_point_index()
{
  for (uint32_t point_index = 0; point_index < point_count; ++point_index)
  {
    const Point* point = &points[point_index];
    if (point->joint_index == skeleton_state->hover_joint_index)
    {
      return (int32_t)point_index;
    }
  }

  return -1;
}