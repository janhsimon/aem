#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct ToolState;
struct Viewport;

bool generate_scene(struct ToolState* tool_state);
void destroy_scene();

void draw_scene(const mat4 world_matrix, const mat4 viewproj_matrix, bool perspective);

void scene_deselect_all_blocks();

enum SceneGetClosestBlockFilter
{
  SceneGetClosestBlockFilter_All,
  SceneGetClosestBlockFilter_OnlySelected,
  SceneGetClosestBlockFilter_OnlyUnselected,
};
bool scene_get_closest_block_to(struct Viewport* viewport,
                                vec2 mouse_position,
                                enum SceneGetClosestBlockFilter filter,
                                uint32_t* out_block_index,
                                float* out_distance);
bool scene_is_block_selected(uint32_t block_index);
void scene_select_block(uint32_t block_index);
void scene_select_blocks_in_bbox(vec3 min, vec3 max);

bool scene_has_selection();
void scene_get_selection_bbox_world(vec3 min, vec3 max);
void scene_get_selection_first_vertex(vec3 v);

void scene_move_selection(vec3 first_vertex_position);
void scene_scale_selection(vec3 new_min, vec3 new_max);
void scene_rotate_selection(float degrees);

// void scene_add_block(vec3 min, vec3 max);
void scene_duplicate_selection();