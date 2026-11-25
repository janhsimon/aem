#pragma once

typedef struct cgltf_material cgltf_material;
typedef struct cgltf_texture_view cgltf_texture_view;
typedef struct cgltf_texture_transform cgltf_texture_transform;

void get_texture_views_from_glb_material(cgltf_material* material,
                                         cgltf_texture_view** base_color_alpha,
                                         cgltf_texture_view** normal,
                                         cgltf_texture_view** metallic_roughness,
                                         cgltf_texture_view** specular_glossiness,
                                         cgltf_texture_view** occlusion,
                                         cgltf_texture_view** emissive);

void get_texture_transform_from_texture_views(cgltf_texture_view* base_color_alpha,
                                              cgltf_texture_view* normal,
                                              cgltf_texture_view* metallic_roughness,
                                              cgltf_texture_view* specular_glossiness,
                                              cgltf_texture_view* occlusion,
                                              cgltf_texture_view* emissive,
                                              cgltf_texture_transform* transform);