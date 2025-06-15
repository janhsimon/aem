#include "render_material.h"

#include <cgltf/cgltf.h>

#include <cglm/io.h>

void print_render_materials(RenderMaterial* render_materials, uint32_t render_material_count)
{
  for (cgltf_size material_index = 0; material_index < render_material_count; ++material_index)
  {
    RenderMaterial* render_material = &render_materials[material_index];

    printf("Render material #%llu \"%s\":\n", material_index,
           render_material->material ? render_material->material->name : "Default");
    printf("\tBase color texture index: %u\n", render_material->base_color_texture_index);
    printf("\tNormal texture index: %u\n", render_material->normal_texture_index);
    printf("\tPBR texture index: %u\n", render_material->pbr_texture_index);
    printf("\tType: %s\n", (render_material->type == AEMMaterialType_Opaque) ? "Opaque" : "Transparent");
    printf("\tUV transform:\n");
    glm_mat3_print(render_material->texture_transform, stdout);
  }
}