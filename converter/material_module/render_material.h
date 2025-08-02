#pragma once

#include <aem/model.h>

#include <cglm/types.h>

#include <stdint.h>

typedef struct cgltf_material cgltf_material;

typedef struct
{
  cgltf_material* material;
  enum AEMMaterialType type;
  uint32_t base_color_texture_index, normal_texture_index, pbr_texture_index;
  mat3 texture_transform;
} RenderMaterial;

void print_render_materials(RenderMaterial* render_materials, uint32_t render_material_count);