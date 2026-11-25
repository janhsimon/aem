#pragma once

#include <cglm/types.h>

#include <stdbool.h>

int get_grid_size_count();
int get_grid_size_index();
bool has_grid_size_tooltip();
const char* get_grid_size_label();
const char* get_grid_size_tooltip();
float get_current_grid_size();
float get_min_grid_size();
bool get_render_3d_grid();

void set_render_3d_grid(bool toggle);
void grid_size_down();
void grid_size_up();
void grid_set_index(int index);
void grid_snap(vec3 in, vec3 out);

bool generate_grid();
void destroy_grid();

void draw_grid(const mat4 viewproj_matrix, bool fade_edges);