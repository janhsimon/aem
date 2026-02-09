#pragma once

struct ModelRenderInfo;

enum ModelRenderMode
{
  ModelRenderMode_AllMeshes,
  ModelRenderMode_OpaqueMeshesOnly,
  ModelRenderMode_TransparentMeshesOnly
};

void model_renderer_add_model(struct ModelRenderInfo* model_render_info);

void load_model_renderer();
void free_model_renderer();

void start_model_rendering();
void render_model(struct ModelRenderInfo* model_render_info, enum ModelRenderMode mode);
