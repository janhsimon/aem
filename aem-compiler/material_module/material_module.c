#include "material_module.h"

#include "material_inspector.h"
#include "output_texture.h"
#include "render_material.h"
#include "render_texture.h"
#include "texture_processor.h"
#include "texture_transform.h"

#include "geometry_module/geometry_module.h"

#include <config.h>

#include <cglm/mat3.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static RenderMaterial* render_materials = NULL;
static cgltf_size render_material_count = 0;

static RenderTexture* render_textures = NULL;
static OutputTexture* output_textures = NULL;
static cgltf_size texture_count = 0;

void mat_create(const cgltf_data* input_file, const char* path)
{
  // Count the number of required render materials
  bool mesh_without_material_found = false;
  render_material_count = input_file->materials_count;
  for (cgltf_size mesh_index = 0; mesh_index < input_file->meshes_count; ++mesh_index)
  {
    const cgltf_mesh* mesh = &input_file->meshes[mesh_index];
    for (cgltf_size primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index)
    {
      const cgltf_primitive* primitive = &mesh->primitives[primitive_index];
      if (geo_is_primitive_valid(primitive) && !primitive->material)
      {
        ++render_material_count;
        mesh_without_material_found = true;
        break;
      }
    }

    if (mesh_without_material_found)
    {
      break;
    }
  }

  // Allocate and default initialize the render material list
  render_materials = malloc(sizeof(*render_materials) * render_material_count);

  // Allocate and zero out the render texture list, also allocate the output texture list
  {
    texture_count = render_material_count * 3;

    {
      const cgltf_size size = sizeof(*render_textures) * texture_count;
      render_textures = malloc(size);
      memset(render_textures, 0, size);
    }

    {
      const cgltf_size size = sizeof(*output_textures) * texture_count;
      output_textures = malloc(size);
    }
  }

  // Populate the render materials and textures
  texture_count = 0;
  for (cgltf_size material_index = 0; material_index < input_file->materials_count; ++material_index)
  {
    cgltf_material* material = &input_file->materials[material_index];
    RenderMaterial* render_material = &render_materials[material_index];

    render_material->material = material;

    // Retrieve texture views for this material
    cgltf_texture_view *base_color_alpha_texture_view, *normal_texture_view, *metallic_roughness_texture_view,
      *specular_glossiness_texture_view, *occlusion_texture_view, *emissive_texture_view;
    get_texture_views_from_glb_material(material, &base_color_alpha_texture_view, &normal_texture_view,
                                        &metallic_roughness_texture_view, &specular_glossiness_texture_view,
                                        &occlusion_texture_view, &emissive_texture_view);

    // Base color/alpha
    {
      // Retrieve color
      vec4 color = GLM_VEC4_ONE_INIT;
      if (material->has_pbr_metallic_roughness)
      {
        glm_vec4_make(material->pbr_metallic_roughness.base_color_factor, color);
      }
      else if (material->has_pbr_specular_glossiness)
      {
        glm_vec4_make(material->pbr_specular_glossiness.diffuse_factor, color);
      }

      // Retrieve alpha mode
      AlphaMode alpha_mode = AlphaMode_Opaque;
      float alpha_mask_threshold = 0.5f;
      if (material->alpha_mode == cgltf_alpha_mode_blend)
      {
        alpha_mode = AlphaMode_Blend;
      }
      else if (material->alpha_mode == cgltf_alpha_mode_mask)
      {
        alpha_mode = AlphaMode_Mask;
        alpha_mask_threshold = material->alpha_cutoff;
      }

      const cgltf_texture* texture = base_color_alpha_texture_view ? base_color_alpha_texture_view->texture : NULL;
      const int32_t existing_index =
        get_existing_base_color_render_texture_index(texture, color, alpha_mode, alpha_mask_threshold, render_textures,
                                                     texture_count);
      if (existing_index < 0)
      {
        add_base_color_render_texture(texture, color, alpha_mode, alpha_mask_threshold,
                                      &render_textures[texture_count]);
        render_material->base_color_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->base_color_texture_index = (uint32_t)existing_index;
      }

      render_material->type = (alpha_mode == AlphaMode_Opaque) ? AEMMaterialType_Opaque : AEMMaterialType_Transparent;
    }

    // Normal
    {
      const cgltf_texture* texture = normal_texture_view ? normal_texture_view->texture : NULL;
      const int32_t existing_index = get_existing_normal_render_texture_index(texture, render_textures, texture_count);
      if (existing_index < 0)
      {
        add_normal_render_texture(texture, &render_textures[texture_count]);
        render_material->normal_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->normal_texture_index = (uint32_t)existing_index;
      }
    }

    // PBR
    {
      // Retrieve factors, workflow and metallic/roughness texture
      vec4 factors;
      PBRWorkflow workflow = PBRWorkflow_MetallicRoughness;
      cgltf_texture* metallic_roughness_texture = NULL;
      {
        if (material->has_pbr_metallic_roughness)
        {
          const cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;
          factors[0] = pbr->roughness_factor;
          factors[2] = pbr->metallic_factor;

          metallic_roughness_texture =
            metallic_roughness_texture_view ? metallic_roughness_texture_view->texture : NULL;
        }
        else if (material->has_pbr_specular_glossiness)
        {
          workflow = PBRWorkflow_SpecularGlossiness;

          cgltf_pbr_specular_glossiness* pbr = &material->pbr_specular_glossiness;
          factors[0] = material->pbr_specular_glossiness.glossiness_factor; // Roughness-equivalent
          factors[2] = glm_vec3_max(pbr->specular_factor);                  // Metallic-equivalent

          metallic_roughness_texture =
            specular_glossiness_texture_view ? specular_glossiness_texture_view->texture : NULL;
        }

        factors[1] = occlusion_texture_view ? occlusion_texture_view->scale : 1.0f; // Occlusion
        factors[3] = emissive_texture_view ? emissive_texture_view->scale : 0.0f;   // Emissive
      }

      const cgltf_texture* occlusion_texture = occlusion_texture_view ? occlusion_texture_view->texture : NULL;
      const cgltf_texture* emissive_texture = emissive_texture_view ? emissive_texture_view->texture : NULL;

      const int32_t existing_index =
        get_existing_pbr_render_texture_index(metallic_roughness_texture, occlusion_texture, emissive_texture, factors,
                                              workflow, render_textures, texture_count);
      if (existing_index < 0)
      {
        add_pbr_render_texture(metallic_roughness_texture, occlusion_texture, emissive_texture, factors, workflow,
                               &render_textures[texture_count]);
        render_material->pbr_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->pbr_texture_index = (uint32_t)existing_index;
      }
    }

    cgltf_texture_transform texture_transform;
    get_texture_transform_from_texture_views(base_color_alpha_texture_view, normal_texture_view,
                                             metallic_roughness_texture_view, specular_glossiness_texture_view,
                                             occlusion_texture_view, emissive_texture_view, &texture_transform);
    transform_to_mat3(&texture_transform, render_material->texture_transform);
  }

  // Add the special last default material if it is needed
  if (mesh_without_material_found)
  {
    RenderMaterial* render_material = &render_materials[input_file->materials_count];

    render_material->material = NULL;
    render_material->type = AEMMaterialType_Opaque;
    glm_mat3_identity(render_material->texture_transform);

    // Base color/alpha
    {
      vec4 color = GLM_VEC4_ONE_INIT;
      AlphaMode alpha_mode = AlphaMode_Opaque;
      float alpha_mask_threshold = 0.5f;

      const int32_t existing_index =
        get_existing_base_color_render_texture_index(NULL, color, alpha_mode, alpha_mask_threshold, render_textures,
                                                     texture_count);
      if (existing_index < 0)
      {
        add_base_color_render_texture(NULL, color, alpha_mode, alpha_mask_threshold, &render_textures[texture_count]);
        render_material->base_color_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->base_color_texture_index = (uint32_t)existing_index;
      }
    }

    // Normal
    {
      const int32_t existing_index = get_existing_normal_render_texture_index(NULL, render_textures, texture_count);
      if (existing_index < 0)
      {
        add_normal_render_texture(NULL, &render_textures[texture_count]);
        render_material->normal_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->normal_texture_index = (uint32_t)existing_index;
      }
    }

    // PBR
    {
      vec4 factors = { 0.5f, 1.0f, 0.0f, 0.0f };
      PBRWorkflow workflow = PBRWorkflow_MetallicRoughness;
      const int32_t existing_index =
        get_existing_pbr_render_texture_index(NULL, NULL, NULL, factors, workflow, render_textures, texture_count);
      if (existing_index < 0)
      {
        add_pbr_render_texture(NULL, NULL, NULL, factors, workflow, &render_textures[texture_count]);
        render_material->pbr_texture_index = (uint32_t)texture_count;
        ++texture_count;
      }
      else
      {
        render_material->pbr_texture_index = (uint32_t)existing_index;
      }
    }
  }

#ifdef PRINT_MATERIALS
  print_render_materials(render_materials, render_material_count);
#endif

#ifdef PRINT_TEXTURES
  print_render_textures(render_textures, texture_count);
#endif

  process_textures(path, render_textures, output_textures, texture_count);

#ifdef PRINT_TEXTURES
  print_output_textures(output_textures, texture_count);
#endif
}

void mat_free()
{
  free(output_textures);

  free(render_textures);
  free(render_materials);
}

bool mat_get_texture_transform_for_material(const cgltf_material* material, mat3 transform)
{
  for (cgltf_size material_index = 0; material_index < render_material_count; ++material_index)
  {
    RenderMaterial* render_material = &render_materials[material_index];

    if (render_material->material == material)
    {
      glm_mat3_copy(render_material->texture_transform, transform);
      return true;
    }
  }

  return false;
}

uint64_t mat_calculate_image_buffer_size()
{
  uint64_t size = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const OutputTexture* texture = &output_textures[texture_index];
    size += (uint64_t)texture->data_size;
  }

  return size;
}

uint32_t mat_get_texture_count()
{
  return (uint32_t)texture_count;
}

uint32_t mat_get_material_count()
{
  return (uint32_t)render_material_count;
}

void mat_write_image_buffer(FILE* output_file)
{
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const OutputTexture* texture = &output_textures[texture_index];
    fwrite(texture->data, texture->data_size, 1, output_file);
  }
}

void mat_write_textures(FILE* output_file)
{
  uint64_t offset = 0;
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const OutputTexture* output_texture = &output_textures[texture_index];
    const RenderTexture* render_texture = &render_textures[texture_index];

    fwrite(&offset, sizeof(offset), 1, output_file);
    fwrite(&output_texture->base_width, sizeof(output_texture->base_width), 1, output_file);
    fwrite(&output_texture->base_height, sizeof(output_texture->base_height), 1, output_file);

    enum AEMTextureWrapMode mode[2];
    mode[0] = cgltf_texture_wrap_mode_to_aem(render_texture->wrap_mode[0]);
    mode[1] = cgltf_texture_wrap_mode_to_aem(render_texture->wrap_mode[1]);

    fwrite(&mode[0], sizeof(mode[0]), 1, output_file);
    fwrite(&mode[1], sizeof(mode[1]), 1, output_file);

    fwrite(&output_texture->channel_count, sizeof(output_texture->channel_count), 1, output_file);
    fwrite(&output_texture->compression, sizeof(output_texture->compression), 1, output_file);

    offset += (uint64_t)output_texture->data_size;
  }
}

void mat_write_materials(FILE* output_file)
{
  for (cgltf_size material_index = 0; material_index < render_material_count; ++material_index)
  {
    const RenderMaterial* material = &render_materials[material_index];

    fwrite(&material->base_color_texture_index, sizeof(material->base_color_texture_index), 1, output_file);
    fwrite(&material->normal_texture_index, sizeof(material->normal_texture_index), 1, output_file);
    fwrite(&material->pbr_texture_index, sizeof(material->pbr_texture_index), 1, output_file);

    fwrite(&material->type, sizeof(material->type), 1, output_file);
  }
}