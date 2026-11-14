#pragma once

#include <cglm/types.h>

#include <stdbool.h>
#include <stdint.h>

struct Viewport;

void generate_map();
void destroy_map();

uint32_t map_get_block_count();
void* map_get_raw_vertex_data();

void map_get_block_color(uint32_t block_index, vec3 color);
void map_get_block_center(uint32_t block_index, vec3 center);

void map_deselect_all_blocks();

uint32_t map_get_last_added_block_index();

enum MapGetClosestBlockSelectionFilter
{
  MapGetClosestBlockSelectionFilter_All,
  MapGetClosestBlockSelectionFilter_OnlySelected,
  MapGetClosestBlockSelectionFilter_OnlyUnselected,
};
enum MapGetClosestBlockAreaFilter
{
  MapGetClosestBlockAreaFilter_OnlyCenter,
  MapGetClosestBlockAreaFilter_OnlyEdges,
  MapGetClosestBlockAreaFilter_All
};
bool map_get_closest_block_to(struct Viewport* viewport,
                              vec2 mouse_position,
                              enum MapGetClosestBlockSelectionFilter selection_filter,
                              enum MapGetClosestBlockAreaFilter area_filter,
                              uint32_t* out_block_index,
                              float* out_distance);
bool map_is_block_selected(uint32_t block_index);
void map_select_block(uint32_t block_index);
void map_select_blocks_in_bbox(vec3 min, vec3 max);
bool map_is_mouse_in_selection_bbox(struct Viewport* viewport, vec2 mouse_position);

bool map_has_selection();
void map_get_selection_bbox_world(vec3 min, vec3 max);
void map_get_selection_first_vertex(vec3 v);

void map_move_selection(vec3 first_vertex_position);
void map_nudge_selection(const struct Viewport* viewport, vec2 direction);
void map_scale_selection(vec3 new_min, vec3 new_max);
void map_rotate_selection(float degrees);

void map_add_block(vec3 min, vec3 max); // In worldspace
void map_duplicate_selection();
void map_delete_selection();