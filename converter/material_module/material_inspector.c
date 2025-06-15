#include "material_inspector.h"

#include "texture_transform.h"

#include <cgltf/cgltf.h>

#include <assert.h>
#include <stdbool.h>

void get_texture_views_from_glb_material(cgltf_material* material,
                                         cgltf_texture_view** base_color_alpha,
                                         cgltf_texture_view** normal,
                                         cgltf_texture_view** metallic_roughness,
                                         cgltf_texture_view** specular_glossiness,
                                         cgltf_texture_view** occlusion,
                                         cgltf_texture_view** emissive)
{
  *base_color_alpha = *normal = *metallic_roughness = *specular_glossiness = *occlusion = *emissive = NULL;

  if (material->normal_texture.texture)
  {
    *normal = &material->normal_texture;
  }

  if (material->occlusion_texture.texture)
  {
    *occlusion = &material->occlusion_texture;
  }

  if (material->emissive_texture.texture)
  {
    *emissive = &material->emissive_texture;
  }

  assert(material->has_pbr_metallic_roughness != material->has_pbr_specular_glossiness);

  if (material->has_pbr_metallic_roughness)
  {
    cgltf_pbr_metallic_roughness* pbr = &material->pbr_metallic_roughness;

    if (pbr->base_color_texture.texture)
    {
      *base_color_alpha = &pbr->base_color_texture;
    }

    if (pbr->metallic_roughness_texture.texture)
    {
      *metallic_roughness = &pbr->metallic_roughness_texture;
    }
  }
  else if (material->has_pbr_specular_glossiness)
  {
    cgltf_pbr_specular_glossiness* pbr = &material->pbr_specular_glossiness;

    if (pbr->diffuse_texture.texture)
    {
      *base_color_alpha = &pbr->diffuse_texture;
    }

    if (pbr->specular_glossiness_texture.texture)
    {
      *specular_glossiness = &pbr->specular_glossiness_texture;
    }
  }
}

void get_texture_transform_from_texture_views(cgltf_texture_view* base_color_alpha,
                                              cgltf_texture_view* normal,
                                              cgltf_texture_view* metallic_roughness,
                                              cgltf_texture_view* specular_glossiness,
                                              cgltf_texture_view* occlusion,
                                              cgltf_texture_view* emissive,
                                              cgltf_texture_transform* transform)
{
  make_identity_transform(transform);

  bool found_transform = false;
  if (base_color_alpha)
  {
    if (base_color_alpha)
    {
      if (base_color_alpha->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&base_color_alpha->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&base_color_alpha->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }

    if (normal)
    {
      if (normal->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&normal->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&normal->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }

    if (metallic_roughness)
    {
      if (metallic_roughness->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&metallic_roughness->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&metallic_roughness->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }

    if (specular_glossiness)
    {
      if (specular_glossiness->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&specular_glossiness->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&specular_glossiness->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }

    if (occlusion)
    {
      if (occlusion->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&occlusion->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&occlusion->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }

    if (emissive)
    {
      if (emissive->has_transform)
      {
        if (!found_transform)
        {
          copy_transforms(&emissive->transform, transform);
          found_transform = true;
        }
        else
        {
          assert(compare_transforms(&emissive->transform, transform));
        }
      }
      else
      {
        assert(!found_transform);
      }
    }
  }
}