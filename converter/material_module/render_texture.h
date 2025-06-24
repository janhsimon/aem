#pragma once

#include <cglm/vec4.h>

#include <cgltf/cgltf.h>

typedef enum
{
  RenderTextureType_BaseColor,
  RenderTextureType_Normal,
  RenderTextureType_PBR // Roughness, occlusion, metalness, emissive
} RenderTextureType;

typedef enum
{
  AlphaMode_Opaque,
  AlphaMode_Blend,
  AlphaMode_Mask
} AlphaMode;

typedef enum
{
  PBRWorkflow_MetallicRoughness,
  PBRWorkflow_SpecularGlossiness
} PBRWorkflow;

struct RenderTexture
{
  RenderTextureType type;

  union
  {
    struct
    {
      cgltf_image* image;
      vec4 color; // RGBA
      AlphaMode alpha_mode;
      float alpha_mask_threshold;
    } base_color;

    struct
    {
      cgltf_image* image;
    } normal;

    struct
    {
      cgltf_image *metallic_roughness_image, *occlusion_image, *emissive_image;
      vec4 factors; // R: roughness, G: occlusion, B: metallic, A: emissive
      PBRWorkflow workflow;
    } pbr;
  };

  cgltf_wrap_mode wrap_mode[2]; // 0: x, 1: y
};
typedef struct RenderTexture RenderTexture;

void print_render_textures(const RenderTexture* render_textures, uint32_t texture_count);

enum AEMTextureWrapMode cgltf_texture_wrap_mode_to_aem(cgltf_wrap_mode wrap_mode);

int32_t get_existing_base_color_render_texture_index(const cgltf_texture* texture,
                                                     vec4 color,
                                                     AlphaMode alpha_mode,
                                                     float alpha_mask_threshold,
                                                     RenderTexture* render_textures,
                                                     cgltf_size length);

int32_t get_existing_normal_render_texture_index(const cgltf_texture* texture,
                                                 RenderTexture* render_textures,
                                                 cgltf_size length);

int32_t get_existing_pbr_render_texture_index(const cgltf_texture* metallic_roughness_texture,
                                              const cgltf_texture* occlusion_texture,
                                              const cgltf_texture* emissive_texture,
                                              vec4 factors,
                                              PBRWorkflow workflow,
                                              RenderTexture* render_textures,
                                              cgltf_size length);

void add_base_color_render_texture(const cgltf_texture* texture,
                                   vec4 color,
                                   AlphaMode alpha_mode,
                                   float alpha_mask_threshold,
                                   RenderTexture* destination);

void add_normal_render_texture(const cgltf_texture* texture, RenderTexture* destination);

void add_pbr_render_texture(const cgltf_texture* metallic_roughness_texture,
                            const cgltf_texture* occlusion_texture,
                            const cgltf_texture* emissive_texture,
                            vec4 factors,
                            PBRWorkflow workflow,
                            RenderTexture* destination);