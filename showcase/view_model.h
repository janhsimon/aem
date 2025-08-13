#pragma once

#include <stdbool.h>

struct AEMModel;

bool load_view_model(const struct AEMModel* model);
void update_view_model(bool moving, float delta_time);
void prepare_view_model_rendering(float aspect);
void free_view_model();