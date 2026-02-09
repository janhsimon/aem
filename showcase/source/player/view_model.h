#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct ModelRenderInfo;

bool load_view_model();

void update_view_model(struct Preferences* preferences, bool moving, float delta_time);

struct ModelRenderInfo* get_view_model_render_info();

void view_model_get_world_matrix(struct Preferences* preferences, mat4 world_matrix);
void prepare_view_model_rendering();
void free_view_model();

int view_model_get_ammo();

void view_model_respawn();