#include "hud.h"

#include "debug_manager.h"
#include "preferences.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static bool debug_window_focus = false;
static bool show_shadow_map_window = false, show_view_space_normals_window = false, show_ssao_window = false;

bool has_debug_window_focus()
{
  return debug_window_focus;
}

bool get_show_shadow_map_window()
{
  return show_shadow_map_window;
}

bool get_show_view_space_normals_window()
{
  return show_view_space_normals_window;
}

bool get_show_ssao_window()
{
  return show_ssao_window;
}

static void update_particle_system(struct ParticleSystemPreferences* preferences)
{
  igSliderInt("Particle count", &preferences->particle_count, 0, 10000, "%d", ImGuiSliderFlags_None);
  igCheckbox("Additive", &preferences->additive);
  igColorEdit3("Particle color", preferences->tint, ImGuiColorEditFlags_None);
  igSliderFloat("Direction spread", &preferences->direction_spread, 0.0f, 360.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Emitter radius", &preferences->radius, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Gravity", &preferences->gravity, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);

  // Opacity
  igSliderFloat("Particle opacity", &preferences->opacity, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Particle opacity spread", &preferences->opacity_spread, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Particle opacity falloff", &preferences->opacity_falloff, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);

  // Scale
  igSliderFloat("Particle scale", &preferences->scale, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Particle scale spread", &preferences->scale_spread, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Particle scale falloff", &preferences->scale_falloff, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
}

void update_debug_window(struct Preferences* preferences, uint32_t screen_width, uint32_t screen_height)
{
  igSetNextWindowSizeConstraints((ImVec2){ screen_width / 4.0f, screen_height / 2.0f },
                                 (ImVec2){ screen_width, screen_height }, NULL, NULL);

  bool open = true;
  igBegin("Debug", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar);

  if (igCollapsingHeader_TreeNodeFlags("Debugging", ImGuiTreeNodeFlags_None))
  {
    igCheckbox("Debug render", &preferences->debug_render);
    if (igButton("Clear lines", (ImVec2){ 0.0f, 0.0f }))
    {
      clear_debug_lines();
    }

    igCheckbox("Show player information", &preferences->show_player_info);
    igCheckbox("Infinite ammo", &preferences->infinite_ammo);
    igCheckbox("No clip", &preferences->no_clip);

    igCheckbox("Show shadow map", &show_shadow_map_window);
    igCheckbox("Show view-space normals", &show_view_space_normals_window);
    igCheckbox("Show SSAO", &show_ssao_window);
  }

  if (igCollapsingHeader_TreeNodeFlags("AI", ImGuiTreeNodeFlags_None))
  {
    igCheckbox("Walking", &preferences->ai_walking);
    igCheckbox("Turning", &preferences->ai_turning);
    igCheckbox("Dying", &preferences->ai_dying);
    igCheckbox("Shooting", &preferences->ai_shooting);
  }

  if (igCollapsingHeader_TreeNodeFlags("Audio", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Master Volume", &preferences->master_volume, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Camera", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Field of view##Camera", &preferences->camera_fov, 0.0f, 180.0f, "%f", ImGuiSliderFlags_None);
    igColorEdit3("Background color##Camera", preferences->camera_background_color, ImGuiColorEditFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Lighting", ImGuiTreeNodeFlags_None))
  {
    if (igTreeNode_Str("Ambient"))
    {
      igColorEdit3("Color##Ambient", preferences->ambient_color, ImGuiColorEditFlags_None);
      igSliderFloat("Intensity##Ambient", &preferences->ambient_intensity, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);

      igTreePop();
    }

    if (igTreeNode_Str("Directional"))
    {
      igColorEdit3("Color##Directional", preferences->light_color, ImGuiColorEditFlags_None);
      igSliderFloat("Intensity##Directional", &preferences->light_intensity, 0.0f, 1000.0f, "%f",
                    ImGuiSliderFlags_None);
      igSliderFloat3("Direction", preferences->light_dir, -1.0f, 1.0f, "%f", ImGuiSliderFlags_None);

      igTreePop();
    }
  }

  if (igCollapsingHeader_TreeNodeFlags("View model", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat3("Position", preferences->view_model_position, -10.0f, 10.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Scale", &preferences->view_model_scale, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Field of view##ViewModel", &preferences->view_model_fov, 0.0f, 180.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Tilt", &preferences->view_model_tilt, -90.0f, 90.0f, "%f", ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("HUD", ImGuiTreeNodeFlags_None))
  {
    igColorEdit4("Foreground color##HUD", preferences->hud_foreground_color, ImGuiColorEditFlags_None);
    igColorEdit4("Background color##HUD", preferences->hud_background_color, ImGuiColorEditFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Particle systems", ImGuiTreeNodeFlags_None))
  {
    if (igTreeNode_Str("Smoke"))
    {
      update_particle_system(&preferences->smoke_particle_system);
      igTreePop();
    }

    if (igTreeNode_Str("Shrapnel"))
    {
      update_particle_system(&preferences->shrapnel_particle_system);
      igTreePop();
    }

    if (igTreeNode_Str("Muzzleflash"))
    {
      update_particle_system(&preferences->muzzleflash_particle_system);
      igTreePop();
    }

    if (igTreeNode_Str("Blood"))
    {
      update_particle_system(&preferences->blood_particle_system);
      igTreePop();
    }
  }

  if (igCollapsingHeader_TreeNodeFlags("Tracer", ImGuiTreeNodeFlags_None))
  {
    igColorEdit4("Color##Tracer", preferences->tracer_color, ImGuiColorEditFlags_None);
    igSliderFloat("Thickness##Tracer", &preferences->tracer_thickness, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Length##Tracer", &preferences->tracer_length, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Speed##Tracer", &preferences->tracer_speed, 0.0f, 1000.0f, "%f", ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Ambient occlusion", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Radius##SSAO", &preferences->ssao_radius, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Bias##SSAO", &preferences->ssao_bias, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Strength##SSAO", &preferences->ssao_strength, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);

    igCheckbox("Blur##SSAO", &preferences->ssao_blur);
    igSliderFloat("Blur depth sigma##SSAO", &preferences->ssao_blur_depth_sigma, 0.0f, 1.0f, "%f",
                  ImGuiSliderFlags_Logarithmic);
    igSliderFloat("Blur radius##SSAO", &preferences->ssao_blur_radius, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  }

  debug_window_focus = igIsWindowFocused(ImGuiFocusedFlags_AnyWindow) || igIsWindowHovered(ImGuiHoveredFlags_AnyWindow);

  igEnd();
}