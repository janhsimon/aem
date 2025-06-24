#include "render_texture.h"

#include <aem/aem.h>

#include <cglm/common.h>

#include <assert.h>
#include <stdint.h>

static const char* texture_type_to_string(RenderTextureType type)
{
  if (type == RenderTextureType_BaseColor)
  {
    return "Base color";
  }
  else if (type == RenderTextureType_Normal)
  {
    return "Normal";
  }

  return "PBR";
}

static const char* texture_alpha_mode_mode_to_string(AlphaMode mode)
{
  if (mode == AlphaMode_Opaque)
  {
    return "Opaque";
  }
  else if (mode == AlphaMode_Blend)
  {
    return "Blend";
  }

  return "Mask";
}

static const char* texture_wrap_mode_to_string(enum cgltf_wrap_mode mode)
{
  if (mode == cgltf_wrap_mode_mirrored_repeat)
  {
    return "Mirrored repeat";
  }
  else if (mode == cgltf_wrap_mode_clamp_to_edge)
  {
    return "Clamp to edge";
  }

  return "Repeat";
}

static void print_image(const cgltf_image* image, const char* name)
{
  printf("\t%s image: \"%s\"\n", name, image->buffer_view ? image->buffer_view->name : image->uri);
  printf("\tLocation: %s\n", image->buffer_view ? "Internal" : "External");
}

void print_render_textures(const RenderTexture* render_textures, uint32_t texture_count)
{
  for (cgltf_size texture_index = 0; texture_index < texture_count; ++texture_index)
  {
    const RenderTexture* texture = &render_textures[texture_index];

    printf("Render texture #%llu:\n", texture_index);

    printf("\tType: %s\n", texture_type_to_string(texture->type));

    if (texture->type == RenderTextureType_BaseColor)
    {
      const cgltf_image* image = texture->base_color.image;
      if (image)
      {
        print_image(image, "Base color");
        printf("\tMix color: [ %.2f, %.2f, %.2f, %.2f ]\n", texture->base_color.color[0], texture->base_color.color[1],
               texture->base_color.color[2], texture->base_color.color[3]);
      }
      else
      {
        printf("\tBase color: [ %.2f, %.2f, %.2f, %.2f ]\n", texture->base_color.color[0], texture->base_color.color[1],
               texture->base_color.color[2], texture->base_color.color[3]);
      }

      printf("\tAlpha mode: %s\n", texture_alpha_mode_mode_to_string(texture->base_color.alpha_mode));
      if (texture->base_color.alpha_mode == AlphaMode_Mask)
      {
        printf("\tMask threshold: %.2f\n", texture->base_color.alpha_mask_threshold);
      }
    }
    else if (texture->type == RenderTextureType_Normal)
    {
      const cgltf_image* image = texture->normal.image;
      if (image)
      {
        print_image(image, "Normal");
      }
      else
      {
        printf("\tNormal values: [ 0.5, 0.5 ]\n");
      }
    }
    else if (texture->type == RenderTextureType_PBR)
    {
      const cgltf_image* metallic_roughness = texture->pbr.metallic_roughness_image;
      if (metallic_roughness)
      {
        print_image(metallic_roughness, "Metallic/roughness");
        printf("\tRoughness factor: %.2f\n", texture->pbr.factors[0]);
        printf("\tMetallic factor: %.2f\n", texture->pbr.factors[2]);
      }
      else
      {
        printf("\tRoughness: %.2f\n", texture->pbr.factors[0]);
        printf("\tMetallic: %.2f\n", texture->pbr.factors[2]);
      }

      const cgltf_image* occlusion = texture->pbr.occlusion_image;
      if (occlusion)
      {
        print_image(occlusion, "Occlusion");
        printf("\tOcclusion factor: %.2f\n", texture->pbr.factors[1]);
      }
      else
      {
        printf("\tOcclusion: %.2f\n", texture->pbr.factors[1]);
      }

      const cgltf_image* emissive = texture->pbr.emissive_image;
      if (emissive)
      {
        print_image(emissive, "Emissive");
        printf("\tEmissive factor: %.2f\n", texture->pbr.factors[3]);
      }
      else
      {
        printf("\tEmissive: %.2f\n", texture->pbr.factors[3]);
      }

      printf("\tPBR workflow: ");
      if (texture->pbr.workflow == PBRWorkflow_MetallicRoughness)
      {
        printf("Metallic/roughness\n");
      }
      else if (texture->pbr.workflow == PBRWorkflow_SpecularGlossiness)
      {
        printf("Specular/glossiness\n");
      }
    }

    printf("\tWrap mode: [ %s, %s ]\n", texture_wrap_mode_to_string(texture->wrap_mode[0]),
           texture_wrap_mode_to_string(texture->wrap_mode[1]));
  }
}

enum AEMTextureWrapMode cgltf_texture_wrap_mode_to_aem(cgltf_wrap_mode wrap_mode)
{
  if (wrap_mode == cgltf_wrap_mode_mirrored_repeat)
  {
    return AEMTextureWrapMode_MirroredRepeat;
  }
  else if (wrap_mode == cgltf_wrap_mode_clamp_to_edge)
  {
    return AEMTextureWrapMode_ClampToEdge;
  }

  assert(wrap_mode == cgltf_wrap_mode_repeat);
  return AEMTextureWrapMode_Repeat;
}

static bool
compare_render_texture_image(const cgltf_texture* texture, const cgltf_image* image, cgltf_wrap_mode wrap_mode[2])
{
  if (!texture && !image)
  {
    return true;
  }

  if ((texture && !image) || (!texture && image))
  {
    return false;
  }

  if (texture->image != image)
  {
    return false;
  }

  if (texture->sampler->wrap_s != wrap_mode[0] || texture->sampler->wrap_t != wrap_mode[1])
  {
    return false;
  }

  return true;
}

int32_t get_existing_base_color_render_texture_index(const cgltf_texture* texture,
                                                     vec4 color,
                                                     AlphaMode alpha_mode,
                                                     float alpha_mask_threshold,
                                                     RenderTexture* render_textures,
                                                     cgltf_size length)
{
  for (cgltf_size texture_index = 0; texture_index < length; ++texture_index)
  {
    RenderTexture* existing_texture = &render_textures[texture_index];
    if (existing_texture->type != RenderTextureType_BaseColor)
    {
      continue;
    }

    if (!compare_render_texture_image(texture, existing_texture->base_color.image, existing_texture->wrap_mode))
    {
      continue;
    }

    if (!glm_vec4_eqv_eps(existing_texture->base_color.color, color))
    {
      continue;
    }

    if (existing_texture->base_color.alpha_mode != alpha_mode)
    {
      continue;
    }

    if (alpha_mode == AlphaMode_Mask &&
        fabsf(existing_texture->base_color.alpha_mask_threshold - alpha_mask_threshold) > GLM_FLT_EPSILON)
    {
      continue;
    }

    return (int32_t)texture_index;
  }

  return -1;
}

int32_t get_existing_normal_render_texture_index(const cgltf_texture* texture,
                                                 RenderTexture* render_textures,
                                                 cgltf_size length)
{
  for (cgltf_size texture_index = 0; texture_index < length; ++texture_index)
  {
    RenderTexture* existing_texture = &render_textures[texture_index];
    if (existing_texture->type != RenderTextureType_Normal)
    {
      continue;
    }

    if (!compare_render_texture_image(texture, existing_texture->normal.image, existing_texture->wrap_mode))
    {
      continue;
    }

    return (int32_t)texture_index;
  }

  return -1;
}

int32_t get_existing_pbr_render_texture_index(const cgltf_texture* metallic_roughness_texture,
                                              const cgltf_texture* occlusion_texture,
                                              const cgltf_texture* emissive_texture,
                                              vec4 factors,
                                              PBRWorkflow workflow,
                                              RenderTexture* render_textures,
                                              cgltf_size length)
{
  for (cgltf_size texture_index = 0; texture_index < length; ++texture_index)
  {
    RenderTexture* existing_texture = &render_textures[texture_index];
    if (existing_texture->type != RenderTextureType_PBR)
    {
      continue;
    }

    if (!compare_render_texture_image(metallic_roughness_texture, existing_texture->pbr.metallic_roughness_image,
                                      existing_texture->wrap_mode))
    {
      continue;
    }

    if (!compare_render_texture_image(occlusion_texture, existing_texture->pbr.occlusion_image,
                                      existing_texture->wrap_mode))
    {
      continue;
    }

    if (!compare_render_texture_image(emissive_texture, existing_texture->pbr.emissive_image,
                                      existing_texture->wrap_mode))
    {
      continue;
    }

    if (!glm_vec4_eqv_eps(existing_texture->pbr.factors, factors))
    {
      continue;
    }

    if (existing_texture->pbr.workflow != workflow)
    {
      continue;
    }

    return (int32_t)texture_index;
  }

  return -1;
}

void add_base_color_render_texture(const cgltf_texture* texture,
                                   vec4 color,
                                   AlphaMode alpha_mode,
                                   float alpha_mask_threshold,
                                   RenderTexture* destination)
{
  destination->type = RenderTextureType_BaseColor;

  destination->base_color.image = texture ? texture->image : NULL;
  glm_vec4_copy(color, destination->base_color.color);

  destination->base_color.alpha_mode = alpha_mode;
  destination->base_color.alpha_mask_threshold = alpha_mask_threshold;

  if (texture)
  {
    destination->wrap_mode[0] = texture->sampler->wrap_s;
    destination->wrap_mode[1] = texture->sampler->wrap_t;
  }
  else
  {
    destination->wrap_mode[0] = destination->wrap_mode[1] = cgltf_wrap_mode_repeat;
  }
}

void add_normal_render_texture(const cgltf_texture* texture, RenderTexture* destination)
{
  destination->type = RenderTextureType_Normal;

  destination->normal.image = texture ? texture->image : NULL;

  if (texture)
  {
    destination->wrap_mode[0] = texture->sampler->wrap_s;
    destination->wrap_mode[1] = texture->sampler->wrap_t;
  }
  else
  {
    destination->wrap_mode[0] = destination->wrap_mode[1] = cgltf_wrap_mode_repeat;
  }
}

void add_pbr_render_texture(const cgltf_texture* metallic_roughness_texture,
                            const cgltf_texture* occlusion_texture,
                            const cgltf_texture* emissive_texture,
                            vec4 factors,
                            PBRWorkflow workflow,
                            RenderTexture* destination)
{
  destination->type = RenderTextureType_PBR;

  destination->pbr.metallic_roughness_image = metallic_roughness_texture ? metallic_roughness_texture->image : NULL;
  destination->pbr.occlusion_image = occlusion_texture ? occlusion_texture->image : NULL;
  destination->pbr.emissive_image = emissive_texture ? emissive_texture->image : NULL;
  glm_vec4_copy(factors, destination->pbr.factors);

  destination->pbr.workflow = workflow;

  destination->wrap_mode[0] = destination->wrap_mode[1] = cgltf_wrap_mode_repeat;
  if (metallic_roughness_texture)
  {
    destination->wrap_mode[0] = metallic_roughness_texture->sampler->wrap_s;
    destination->wrap_mode[1] = metallic_roughness_texture->sampler->wrap_t;
  }
  else if (occlusion_texture)
  {
    destination->wrap_mode[0] = occlusion_texture->sampler->wrap_s;
    destination->wrap_mode[1] = occlusion_texture->sampler->wrap_t;
  }
  else if (emissive_texture)
  {
    destination->wrap_mode[0] = emissive_texture->sampler->wrap_s;
    destination->wrap_mode[1] = emissive_texture->sampler->wrap_t;
  }
}