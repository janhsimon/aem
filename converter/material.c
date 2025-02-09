#include "material.h"

#include "config.h"

#include <cglm/affine2d.h>
#include <cglm/io.h>

#include <cgltf/cgltf.h>

void make_uv_transform(cgltf_texture_transform* transform, mat3 uv_transform)
{
  if (!transform)
  {
    glm_mat3_identity(uv_transform);
    return;
  }

  glm_translate2d_make(uv_transform, transform->offset);
  glm_rotate2d(uv_transform, transform->rotation);
  glm_scale2d(uv_transform, transform->scale);
}

void write_materials(const struct cgltf_data* input_file, FILE* output_file)
{
  for (cgltf_size material_index = 0; material_index < input_file->materials_count; ++material_index)
  {
    cgltf_material* material = &input_file->materials[material_index];

    int32_t base_color_texture_index = -1, normal_texture_index = -1, metallic_roughness_texture_index = -1 /*,
             occlusion_texture_index = -1*/
      ;
    cgltf_texture_transform *base_color_texture_transform = NULL, *normal_texture_transform = NULL,
                            *metallic_roughness_texture_transform = NULL, *occlusion_texture_transform = NULL;
    if (material->has_pbr_metallic_roughness)
    {
      cgltf_texture_view* base_color = &material->pbr_metallic_roughness.base_color_texture;
      if (base_color->texture)
      {
        base_color_texture_index = cgltf_texture_index(input_file, base_color->texture);

        if (base_color->has_transform)
        {
          base_color_texture_transform = &base_color->transform;
        }
      }

      /*const cgltf_texture* metallic_roughness_texture =
        material->pbr_metallic_roughness.metallic_roughness_texture.texture;
      if (metallic_roughness_texture)
      {
        metallic_roughness_texture_index = cgltf_texture_index(input_file, metallic_roughness_texture);
      }*/
    }
    else if (material->has_pbr_specular_glossiness)
    {
      cgltf_texture_view* base_color = &material->pbr_specular_glossiness.diffuse_texture;
      if (base_color->texture)
      {
        base_color_texture_index = cgltf_texture_index(input_file, base_color->texture);

        if (base_color->has_transform)
        {
          base_color_texture_transform = &base_color->transform;
        }
      }
    }

    cgltf_texture_view* normal = &material->normal_texture;
    if (normal->texture)
    {
      normal_texture_index = cgltf_texture_index(input_file, normal->texture);

      if (normal->has_transform)
      {
        normal_texture_transform = &normal->transform;
      }
    }

    /*
    const cgltf_texture* occlusion_texture = material->occlusion_texture.texture;
    if (occlusion_texture)
    {
      occlusion_texture_index = cgltf_texture_index(input_file, occlusion_texture);
    }*/

    mat3 base_color_uv_transform;
    {
      fwrite(&base_color_texture_index, sizeof(base_color_texture_index), 1, output_file);

      make_uv_transform(base_color_texture_transform, base_color_uv_transform);
      fwrite(&base_color_uv_transform, sizeof(base_color_uv_transform), 1, output_file);
    }

    mat3 normal_uv_transform;
    {
      fwrite(&normal_texture_index, sizeof(normal_texture_index), 1, output_file);

      make_uv_transform(normal_texture_transform, normal_uv_transform);
      fwrite(&normal_uv_transform, sizeof(normal_uv_transform), 1, output_file);
    }

    mat3 metallic_roughness_uv_transform;
    {
      fwrite(&metallic_roughness_texture_index, sizeof(metallic_roughness_texture_index), 1, output_file);

      make_uv_transform(metallic_roughness_texture_transform, metallic_roughness_uv_transform);
      fwrite(&metallic_roughness_uv_transform, sizeof(metallic_roughness_uv_transform), 1, output_file);
    }

#ifdef PRINT_MATERIALS
    printf("Material #%llu \"%s\":\n", material_index, material->name);

    printf("\tBase color texture index: %d\n", base_color_texture_index);
    //printf("\tBase color uv transform:\n");
    //glm_mat3_print(base_color_uv_transform, stdout);

    printf("\tNormal texture index: %d\n", normal_texture_index);
    //printf("\tNormal uv transform:\n");
    //glm_mat3_print(normal_uv_transform, stdout);

    printf("\tRoughness/Metalness texture index: %d\n", metallic_roughness_texture_index);
    //printf("\tRoughness/Metalness uv transform:\n");
    //glm_mat3_print(metallic_roughness_uv_transform, stdout);

    // printf("\tOcclusion texture index: %d\n", occlusion_texture_index);
#endif
  }
}