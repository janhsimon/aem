#include "hud.h"

#include "debug_manager.h"
#include "preferences.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static bool debug_window_focus = false;
static bool show_shadow_map_window = false, show_view_space_normals_window = false, show_ssao_window = false,
            show_bloom_window = false;

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

bool get_show_bloom_window()
{
  return show_bloom_window;
}

static void update_particle_system(struct ParticleSystemPreferences* preferences)
{
  igSliderInt("Particle count", &preferences->particle_count, 0, 10000, "%d", ImGuiSliderFlags_None);
  igCheckbox("Additive", &preferences->additive);
  igSliderFloat("Particle brightness", &preferences->brightness, 0.0f, 100.0f, "%f", ImGuiSliderFlags_Logarithmic);
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

static void plot_recoil_pattern(const struct RecoilPattern* pattern, uint32_t count, ImVec2 canvas_size)
{
  ImDrawList* draw_list = igGetWindowDrawList();

  ImVec2 p0 = igGetCursorScreenPos(); // top-left
  ImVec2 p1 = { p0.x + canvas_size.x, p0.y + canvas_size.y };

  // Reserve space in the layout
  igInvisibleButton("plot_canvas", canvas_size, 0);

  // Colors
  ImU32 bg_col = igGetColorU32_Vec4((ImVec4){ 0.16f, 0.16f, 0.16f, 1.0f });
  ImU32 grid_col = igGetColorU32_Vec4((ImVec4){ 0.27f, 0.27f, 0.27f, 1.0f });
  ImU32 axis_col = igGetColorU32_Vec4((ImVec4){ 0.47f, 0.47f, 0.47f, 1.0f });
  ImU32 point_col = igGetColorU32_Vec4((ImVec4){ 1.0f, 0.25f, 0.25f, 1.0f });

  // Background
  ImDrawList_AddRectFilled(draw_list, p0, p1, bg_col, 0.0f, 0);

  // Grid
  const int grid_lines = 10;

  for (int i = 0; i <= grid_lines; ++i)
  {
    float t = (float)i / (float)grid_lines;

    float x = p0.x + t * canvas_size.x;
    float y = p0.y + t * canvas_size.y;

    // Vertical
    ImDrawList_AddLine(draw_list, (ImVec2){ x, p0.y }, (ImVec2){ x, p1.y }, grid_col, 1.0f);

    // Horizontal
    ImDrawList_AddLine(draw_list, (ImVec2){ p0.x, y }, (ImVec2){ p1.x, y }, grid_col, 1.0f);
  }

  // X axis at y = 0
  float axis_y = p1.y;
  ImDrawList_AddLine(draw_list, (ImVec2){ p0.x, axis_y }, (ImVec2){ p1.x, axis_y }, axis_col, 2.0f);

  // Y axis at x = 0
  float axis_x = p0.x + canvas_size.x * 0.5f;
  ImDrawList_AddLine(draw_list, (ImVec2){ axis_x, p0.y }, (ImVec2){ axis_x, p1.y }, axis_col, 2.0f);

  // Plot points
  for (int i = 0; i < count; ++i)
  {
    float nx = pattern[i].recoil[0];
    float ny = pattern[i].recoil[1];

    // Normalize to screen space
    float sx = p0.x + ((nx + 1.0f) * 0.5f) * canvas_size.x;
    float sy = p1.y - (ny * canvas_size.y);

    ImDrawList_AddCircleFilled(draw_list, (ImVec2){ sx, sy }, 3.0f, point_col, 12);
  }
}

static void update_weapon(struct WeaponPreferences* preferences)
{
  igSeparatorText("Movement spread");

  igSliderInt("Bullet count", &preferences->bullet_count, 0, 1000, "%u", ImGuiSliderFlags_None);
  igSliderFloat("Fire rate", &preferences->fire_rate, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);

  // Movement spread
  igSliderFloat("Neutral", &preferences->movement_spread_neutral, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Crouch", &preferences->movement_spread_crouch, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Walk", &preferences->movement_spread_walk, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Run", &preferences->movement_spread_run, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Air", &preferences->movement_spread_air, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Grow time", &preferences->movement_spread_grow_time, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Shrink time", &preferences->movement_spread_shrink_time, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);

  // Firing spread
  igSeparatorText("Firing spread");
  igSliderFloat("Scale##FiringSpread", &preferences->firing_spread_scale, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);

  // Recoil
  igSeparatorText("Recoil");

  // Pattern
  {
    static int count = -1;
    if (count < 0)
    {
      count = preferences->bullet_count;
    }

    ImVec2 avail = igGetContentRegionAvail();
    plot_recoil_pattern(preferences->recoil_pattern, count, (ImVec2){ avail.x, avail.x });

    char s[16];
    sprintf(s, "%%d / %u", preferences->bullet_count);
    igSliderInt("Bullet index", &count, 1, preferences->bullet_count, s, ImGuiSliderFlags_None);
  }

  igSliderFloat2("Scale##Recoil", preferences->recoil_scale, -1.0f, 1.0f, "%f", ImGuiSliderFlags_None);

  // View punch
  igSeparatorText("View punch");
  // igSliderFloat("Scale##ViewPunch", &preferences->view_punch_scale, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat2("Amount##ViewPunch", preferences->view_punch, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
  igSliderFloat("Recover speed##ViewPunch", &preferences->view_punch_recover_speed, 0.0f, 1000.0f, "%f",
                ImGuiSliderFlags_None);
}

void update_debug_window(struct Preferences* preferences, uint32_t screen_width, uint32_t screen_height)
{
  igSetNextWindowSizeConstraints((ImVec2){ screen_width / 3.0f, screen_height / 2.0f },
                                 (ImVec2){ screen_width, screen_height }, NULL, NULL);

  igSetNextWindowPos((ImVec2){ 0.0f, 0.0f }, ImGuiCond_Once, (ImVec2){ 0.0f, 0.0f });
  igSetNextWindowSize((ImVec2){ screen_width / 3.0f, screen_height }, ImGuiCond_Once);

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
    igCheckbox("No movement spread", &preferences->no_movement_spread);
    igCheckbox("No firing spread", &preferences->no_firing_spread);
    igCheckbox("No recoil", &preferences->no_recoil);
    igCheckbox("No view punch", &preferences->no_view_punch);

    igCheckbox("Show shadow map", &show_shadow_map_window);
    igCheckbox("Show view-space normals", &show_view_space_normals_window);
    igCheckbox("Show SSAO", &show_ssao_window);
    igCheckbox("Show bloom", &show_bloom_window);
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
    igSliderFloat("Master volume", &preferences->master_volume, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Camera", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Field of view##Camera", &preferences->camera_fov, 0.0f, 180.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Near clip distance##Camera", &preferences->camera_near, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Far clip distance##Camera", &preferences->camera_far, 0.0f, 1000.0f, "%f", ImGuiSliderFlags_None);
    igColorEdit3("Background color##Camera", preferences->camera_background_color, ImGuiColorEditFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Player", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Run speed##Player", &preferences->player_run_speed, 0.0f, 20.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Walk speed##Player", &preferences->player_walk_speed, 0.0f, 20.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Crouch speed##Player", &preferences->player_crouch_speed, 0.0f, 20.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Acceleration##Player", &preferences->player_accel, 0.0f, 2.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Friction##Player", &preferences->player_friction, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Jump strength##Player", &preferences->player_jump_strength, 0.0f, 250.0f, "%f",
                  ImGuiSliderFlags_None);
    igSliderFloat("No clip speed factor##Player", &preferences->player_no_clip_speed_factor, 0.0f, 10.0f, "%f",
                  ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Input", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Mouse sensitivity##Input", &preferences->input_mouse_sensitivity, 0.01f, 100.0f, "%f",
                  ImGuiSliderFlags_None);
  }

  if (igCollapsingHeader_TreeNodeFlags("Physics", ImGuiTreeNodeFlags_None))
  {
    igSliderFloat("Gravity##Physics", &preferences->physics_gravity, 0.0f, 200.0f, "%f", ImGuiSliderFlags_None);
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

  if (igCollapsingHeader_TreeNodeFlags("Shadow mapping", ImGuiTreeNodeFlags_None))
  {
    igCheckbox("Enable##ShadowMapping", &preferences->shadow_mapping_enable);

    if (igTreeNode_Str("Cascade Texture Sizes"))
    {
      static const char* resolution_names[] = { "256px", "512px", "1024px", "2048px", "4096px", "8192px", "16384px" };

      for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
      {
        char name_buffer[128], text_buffer[32];
        sprintf(name_buffer, "Cascade #%u Texture Size##ShadowMapping", cascade_index + 1);
        sprintf(text_buffer, "%upx", preferences->shadow_mapping_cascade_texture_sizes[cascade_index]);

        if (igBeginCombo(name_buffer, text_buffer, ImGuiComboFlags_None))
        {
          for (int i = 0; i < 7; ++i)
          {
            if (igSelectable_Bool(resolution_names[i],
                                  preferences->shadow_mapping_cascade_texture_sizes[cascade_index] == (256 << i), 0,
                                  (struct ImVec2_c){ 0, 0 }))
            {
              preferences->shadow_mapping_cascade_texture_sizes[cascade_index] = (256 << i);
            }
          }

          igEndCombo();
        }
      }

      igTreePop();
    }

    if (igTreeNode_Str("Cascade Splits"))
    {
      igSliderFloat("Cascade Split #1##ShadowMapping", &preferences->shadow_mapping_cascade_splits[0], 0.0f, 1.0f, "%f",
                    ImGuiSliderFlags_Logarithmic);

      igSliderFloat("Cascade Split #2##ShadowMapping", &preferences->shadow_mapping_cascade_splits[1], 0.0f, 1.0f, "%f",
                    ImGuiSliderFlags_Logarithmic);

      igSliderFloat("Cascade Split #3##ShadowMapping", &preferences->shadow_mapping_cascade_splits[2], 0.0f, 1.0f, "%f",
                    ImGuiSliderFlags_Logarithmic);

      igTreePop();
    }

    igSliderFloat("Bias##ShadowMapping", &preferences->shadow_mapping_bias, 0.0f, 25.0f, "%f",
                  ImGuiSliderFlags_Logarithmic);
    igSliderFloat("PCF Radius##ShadowMapping", &preferences->shadow_mapping_pcf_radius, 0.0f, 100.0f, "%f",
                  ImGuiSliderFlags_Logarithmic);
    igSliderInt("PCF Kernel Size##ShadowMapping", &preferences->shadow_mapping_pcf_kernel_size, 1, 25, "%d",
                ImGuiSliderFlags_None);

    igCheckbox("Visualize Cascades##ShadowMapping", &preferences->shadow_mapping_visualize_cascades);
  }

  if (igCollapsingHeader_TreeNodeFlags("Weapons", ImGuiTreeNodeFlags_None))
  {
    if (igTreeNode_Str("CZ"))
    {
      update_weapon(&preferences->weapon_cz);
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

    // Crosshair
    igSeparatorText("Crosshair");
    igCheckbox("Dot", &preferences->hud_crosshair_dot);
    igCheckbox("Lines", &preferences->hud_crosshair_lines);
    igCheckbox("Outline", &preferences->hud_crosshair_outline);

    igBeginDisabled(!preferences->hud_crosshair_outline);
    igColorEdit4("Outline color##HUD", preferences->hud_crosshair_outline_color, ImGuiColorEditFlags_None);
    igEndDisabled();

    igSliderInt("Thickness##HUD", &preferences->hud_crosshair_thickness, 1, 100, "%d", ImGuiSliderFlags_None);

    igBeginDisabled(!preferences->hud_crosshair_lines);
    igSliderInt("Length##HUD", &preferences->hud_crosshair_length, 1, 100, "%d", ImGuiSliderFlags_None);
    igSliderInt("Gap min##HUD", &preferences->hud_crosshair_gap_min, 0, 200, "%d", ImGuiSliderFlags_None);
    igSliderInt("Gap max##HUD", &preferences->hud_crosshair_gap_max, 0, 200, "%d", ImGuiSliderFlags_None);
    igSliderFloat("Spread min##HUD", &preferences->hud_crosshair_spread_min, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderFloat("Spread max##HUD", &preferences->hud_crosshair_spread_max, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
    igSliderInt("Fire expansion##HUD", &preferences->hud_crosshair_fire_expand, 0, 100, "%d", ImGuiSliderFlags_None);
    igEndDisabled();
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
    igSliderFloat("Brightness##Tracer", &preferences->tracer_brightness, 0.0f, 100.0f, "%f",
                  ImGuiSliderFlags_Logarithmic);
    igSliderFloat("Thickness##Tracer", &preferences->tracer_thickness, 0.0f, 1.0f, "%f", ImGuiSliderFlags_Logarithmic);
    igSliderFloat("Length##Tracer", &preferences->tracer_length, 0.0f, 100.0f, "%f", ImGuiSliderFlags_Logarithmic);
    igSliderFloat("Speed##Tracer", &preferences->tracer_speed, 0.0f, 1000.0f, "%f", ImGuiSliderFlags_Logarithmic);
  }

  if (igCollapsingHeader_TreeNodeFlags("Post-processing", ImGuiTreeNodeFlags_None))
  {
    if (igTreeNode_Str("Ambient occlusion"))
    {
      igCheckbox("Enable##SSAO", &preferences->ssao_enable);

      igSliderFloat("Radius##SSAO", &preferences->ssao_radius, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Bias##SSAO", &preferences->ssao_bias, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Strength##SSAO", &preferences->ssao_strength, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);

      igCheckbox("Blur##SSAO", &preferences->ssao_blur_enable);
      igSliderFloat("Blur depth sigma##SSAO", &preferences->ssao_blur_depth_sigma, 0.0f, 1.0f, "%f",
                    ImGuiSliderFlags_Logarithmic);
      igSliderFloat("Blur radius##SSAO", &preferences->ssao_blur_radius, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);

      igTreePop();
    }

    if (igTreeNode_Str("Bloom"))
    {
      igCheckbox("Enable##Bloom", &preferences->bloom_enable);

      igSliderFloat("Threshold##Bloom", &preferences->bloom_threshold, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Soft knee##Bloom", &preferences->bloom_soft_knee, 0.0f, 10.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Intensity##Bloom", &preferences->bloom_intensity, 0.0f, 10.0f, "%f", ImGuiSliderFlags_Logarithmic);

      igTreePop();
    }
  }

  debug_window_focus = igIsWindowFocused(ImGuiFocusedFlags_AnyWindow) || igIsWindowHovered(ImGuiHoveredFlags_AnyWindow);

  igEnd();
}