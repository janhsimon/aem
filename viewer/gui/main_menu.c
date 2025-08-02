#include "main_menu.h"

#include "display_state.h"
#include "scene_state.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

struct DisplayState* display_state;
struct SceneState* scene_state;

static void (*file_open_callback)() = NULL;

void init_main_menu(struct DisplayState* display_state_, struct SceneState* scene_state_, void (*file_open_callback_)())
{
  display_state = display_state_;
  scene_state = scene_state_;

  file_open_callback = file_open_callback_;
}

void update_main_menu()
{
  if (igBeginMainMenuBar())
  {
    if (igBeginMenu("File", true))
    {
      if (igMenuItem_Bool("Open...", "O", false, true))
      {
        file_open_callback();
      }

      igEndMenu();
    }

    if (igBeginMenu("Render Mode", true))
    {
      if (igMenuItem_Bool("Full", "", display_state->render_mode == RenderMode_Full, true))
      {
        display_state->render_mode = RenderMode_Full;
      }

      igSeparator();

      if (igMenuItem_Bool("Vertex Position", "", display_state->render_mode == RenderMode_VertexPosition, true))
      {
        display_state->render_mode = RenderMode_VertexPosition;
      }
      if (igMenuItem_Bool("Vertex Normal", "", display_state->render_mode == RenderMode_VertexNormal, true))
      {
        display_state->render_mode = RenderMode_VertexNormal;
      }
      if (igMenuItem_Bool("Vertex Tangent", "", display_state->render_mode == RenderMode_VertexTangent, true))
      {
        display_state->render_mode = RenderMode_VertexTangent;
      }
      if (igMenuItem_Bool("Vertex Bitangent", "", display_state->render_mode == RenderMode_VertexBitangent, true))
      {
        display_state->render_mode = RenderMode_VertexBitangent;
      }
      if (igMenuItem_Bool("Vertex UV", "", display_state->render_mode == RenderMode_VertexUV, true))
      {
        display_state->render_mode = RenderMode_VertexUV;
      }

      igSeparator();

      if (igMenuItem_Bool("Base Color Texture", "", display_state->render_mode == RenderMode_TextureBaseColor, true))
      {
        display_state->render_mode = RenderMode_TextureBaseColor;
      }

      if (igMenuItem_Bool("Opacity Texture", "", display_state->render_mode == RenderMode_TextureOpacity, true))
      {
        display_state->render_mode = RenderMode_TextureOpacity;
      }

      if (igMenuItem_Bool("Normal Texture", "", display_state->render_mode == RenderMode_TextureNormal, true))
      {
        display_state->render_mode = RenderMode_TextureNormal;
      }

      if (igMenuItem_Bool("Roughness Texture", "", display_state->render_mode == RenderMode_TextureRoughness, true))
      {
        display_state->render_mode = RenderMode_TextureRoughness;
      }

      if (igMenuItem_Bool("Occlusion Texture", "", display_state->render_mode == RenderMode_TextureOcclusion, true))
      {
        display_state->render_mode = RenderMode_TextureOcclusion;
      }

      if (igMenuItem_Bool("Metalness Texture", "", display_state->render_mode == RenderMode_TextureMetalness, true))
      {
        display_state->render_mode = RenderMode_TextureMetalness;
      }

      if (igMenuItem_Bool("Emissive Texture", "", display_state->render_mode == RenderMode_TextureEmissive, true))
      {
        display_state->render_mode = RenderMode_TextureEmissive;
      }

      igSeparator();

      if (igMenuItem_Bool("Combined Normal", "", display_state->render_mode == RenderMode_CombinedNormal, true))
      {
        display_state->render_mode = RenderMode_CombinedNormal;
      }

      igEndMenu();
    }

    if (igBeginMenu("View", true))
    {
      igMenuItem_BoolPtr("Render Transparent", "T", &display_state->render_transparent, true);

      igSeparator();

      // Scene scale
      igSliderInt("##SceneScale", &scene_state->scale, 1, 500, "Scene Scale: %d%%", 0);
      igSeparator();

      // Reset camera pivot
      if (igMenuItem_Bool("Reset Camera Pivot", "P", false, true))
      {
        reset_camera_pivot();
      }

      // Camera fov
      igSliderInt("##CameraFOV", &scene_state->camera_fov, 10, 160, "Camera FOV: %d deg", 0);

      // Auto-rotate camera
      if (igMenuItem_Bool("Auto-Rotate Camera", "R", scene_state->auto_rotate_camera, true))
      {
        scene_state->auto_rotate_camera = !scene_state->auto_rotate_camera;
      }

      igSliderInt("##AutoRotateCameraSpeed", &scene_state->auto_rotate_camera_speed, 1, 500, "Camera Speed: %d%%", 0);

      igSeparator();

      igColorEdit3("Background Color", scene_state->background_color, ImGuiColorEditFlags_NoInputs);

      igColorEdit3("Ambient Color", scene_state->ambient_color, ImGuiColorEditFlags_NoInputs);
      igSliderFloat("##AmbientIntensity", &scene_state->ambient_color[3], 0.0f, 5.0f, "Ambient Intensity: %.2f", 0);

      igColorEdit3("Light Color", scene_state->light_color, ImGuiColorEditFlags_NoInputs);
      igSliderFloat("##LightIntensity", &scene_state->light_color[3], 0.0f, 100.0f, "Light Intensity: %.2f", 0);

      igSeparator();

      // Show flags
      if (igMenuItem_Bool("Show GUI", "U", display_state->show_gui, true))
      {
        display_state->show_gui = !display_state->show_gui;
      }
      if (igMenuItem_Bool("Show Grid", "G", display_state->show_grid, true))
      {
        display_state->show_grid = !display_state->show_grid;
      }
      if (igMenuItem_Bool("Show Skeleton", "S", display_state->show_skeleton, true))
      {
        display_state->show_skeleton = !display_state->show_skeleton;
      }
      if (igMenuItem_Bool("Show Wireframe", "W", display_state->show_wireframe, true))
      {
        display_state->show_wireframe = !display_state->show_wireframe;
      }

      igEndMenu();
    }

    if (igBeginMenu("Postprocessing", true))
    {
      if (igMenuItem_Bool("Linear", "", display_state->postprocessing_mode == PostprocessingMode_Linear, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_Linear;
      }

      if (igMenuItem_Bool("sRGB", "", display_state->postprocessing_mode == PostprocessingMode_sRGB, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB;
      }

      if (igMenuItem_Bool("sRGB + Reinhard", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_Reinhard,
                          true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_Reinhard;
      }

      if (igMenuItem_Bool("sRGB + Reinhard X", "",
                          display_state->postprocessing_mode == PostprocessingMode_sRGB_ReinhardX, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_ReinhardX;
      }

      if (igMenuItem_Bool("sRGB + ACES", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_ACES, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_ACES;
      }

      if (igMenuItem_Bool("sRGB + Filmic", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_Filmic,
                          true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_Filmic;
      }

      igEndMenu();
    }

    igEndMainMenuBar();
  }
}