#pragma once

#include <stdbool.h>

typedef enum
{
  RenderMode_Full,
  RenderMode_VertexPosition,
  RenderMode_VertexNormal,
  RenderMode_VertexTangent,
  RenderMode_VertexBitangent,
  RenderMode_VertexUV,
  RenderMode_TextureBaseColor,
  RenderMode_TextureOpacity,
  RenderMode_TextureNormal,
  RenderMode_TextureRoughness,
  RenderMode_TextureOcclusion,
  RenderMode_TextureMetalness,
  RenderMode_TextureEmissive,
  RenderMode_CombinedNormal
} RenderMode;

typedef enum
{
  PostprocessingMode_Linear,
  PostprocessingMode_sRGB,
  PostprocessingMode_sRGB_Reinhard,
  PostprocessingMode_sRGB_ReinhardX,
  PostprocessingMode_sRGB_ACES,
  PostprocessingMode_sRGB_Filmic,
} PostprocessingMode;

struct DisplayState
{
  bool show_gui;
  bool show_grid;
  bool show_skeleton;
  bool show_wireframe;
  RenderMode render_mode;
  bool render_transparent;
  PostprocessingMode postprocessing_mode;
};