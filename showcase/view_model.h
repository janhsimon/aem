#pragma once

#include <cglm/types.h>

#include <stdbool.h>

struct AEMModel;

bool load_view_model(const struct AEMModel* model);
void update_view_model(bool moving, float delta_time);
void view_model_get_world_matrix(const struct Preferences* preferences, mat4 world_matrix);
void prepare_view_model_rendering(float aspect);
void free_view_model();

int view_model_get_ammo();