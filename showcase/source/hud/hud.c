#include "hud.h"

#include "camera.h"
#include "debug/debug_renderer.h"
#include "hud_damage_indicator.h"
#include "player/player.h"
#include "player/view_model.h"
#include "preferences.h"
#include "window.h"

#include <cglm/util.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui_impl.h>

#include <string.h>

#define FONT_SIZE 40.0f
#define AMMO_OFFSET_X 25.0f
#define AMMO_OFFSET_Y 10.0f

static ImGuiContext* context = NULL;
static ImGuiIO* io = NULL;
static ImFont *font_lambda = NULL, *font_jura_med = NULL;

static bool debug_window_focus = false;

bool load_hud()
{
  context = igCreateContext(NULL);

  io = igGetIO_ContextPtr(context);
  io->IniFilename = NULL;
  io->LogFilename = NULL;

  ImFontAtlas_AddFontDefault(io->Fonts, NULL); // Keep the built-in default font for debug text

  // Load the fonts
  font_lambda = ImFontAtlas_AddFontFromFileTTF(io->Fonts, "fonts/lambda.ttf", FONT_SIZE, NULL, NULL);
  if (!font_lambda)
  {
    return false;
  }

  font_jura_med = ImFontAtlas_AddFontFromFileTTF(io->Fonts, "fonts/jura-medium.ttf", FONT_SIZE, NULL, NULL);
  if (!font_jura_med)
  {
    return false;
  }

  const char* glsl_version = "#version 330 core";
  if (!ImGui_ImplGlfw_InitForOpenGL(get_window(), true))
  {
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init(glsl_version))
  {
    return false;
  }

  if (!load_hud_damage_indicator())
  {
    return false;
  }

  return true;
}

bool has_debug_window_focus()
{
  return debug_window_focus;
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

void update_hud(uint32_t screen_width,
                uint32_t screen_height,
                float delta_time,
                bool debug_mode,
                struct Preferences* preferences)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

  ImDrawList* draw_list = igGetForegroundDrawList_WindowPtr(context->CurrentWindow);

  ImVec4 foreground_color, background_color;
  {
    foreground_color.x = preferences->hud_foreground_color[0];
    foreground_color.y = preferences->hud_foreground_color[1];
    foreground_color.z = preferences->hud_foreground_color[2];
    foreground_color.w = preferences->hud_foreground_color[3];
  }

  {
    background_color.x = preferences->hud_background_color[0];
    background_color.y = preferences->hud_background_color[1];
    background_color.z = preferences->hud_background_color[2];
    background_color.w = preferences->hud_background_color[3];
  }

  const float half_screen_width = (float)(screen_width / 2);
  const float half_screen_height = (float)(screen_height / 2);

  // Crosshair
  if (get_player_health() > 0.0f)
  {
    const float gap_size = 6.0f;
    const float half_gap_size = gap_size / 2.0f;
    const float line_size = 8.0f;

    const ImU32 color = igGetColorU32_Vec4(foreground_color);

    // Left and right
    ImDrawList_AddLine(draw_list, (ImVec2){ half_screen_width - half_gap_size - line_size, half_screen_height },
                       (ImVec2){ half_screen_width - half_gap_size, half_screen_height }, color, 1.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){ half_screen_width + half_gap_size, half_screen_height },
                       (ImVec2){ half_screen_width + half_gap_size + line_size, half_screen_height }, color, 1.0f);

    // Top and bottom
    ImDrawList_AddLine(draw_list, (ImVec2){ half_screen_width, half_screen_height - half_gap_size - line_size },
                       (ImVec2){ half_screen_width, half_screen_height - half_gap_size }, color, 1.0f);
    ImDrawList_AddLine(draw_list, (ImVec2){ half_screen_width, half_screen_height + half_gap_size },
                       (ImVec2){ half_screen_width, half_screen_height + half_gap_size + line_size }, color, 1.0f);
  }

  // Health and ammo displays
  {
    const float ui_scale = screen_height / 720.0f;
    igPushFont(font_lambda, FONT_SIZE * ui_scale);

    igPushStyleColor_Vec4(ImGuiCol_Text, foreground_color);
    igPushStyleColor_Vec4(ImGuiCol_WindowBg, background_color);

    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 12.0f * ui_scale);
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){ 20.0f * ui_scale, 5.0f * ui_scale });

    // Health display
    {
      igSetNextWindowPos((ImVec2){ AMMO_OFFSET_X, screen_height - AMMO_OFFSET_Y }, ImGuiCond_Always,
                         (ImVec2){ 0.0f, 1.0f });

      char s[16];
      sprintf(s, "+ %d   * 100", (int)get_player_health());

      bool open = true;
      igBegin("Health", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
      igText(s);

      igEnd();
    }

    // Ammo display
    {
      igSetNextWindowPos((ImVec2){ screen_width - AMMO_OFFSET_X, screen_height - AMMO_OFFSET_Y }, ImGuiCond_Always,
                         (ImVec2){ 1.0f, 1.0f });

      char s[16];
      sprintf(s, "a %d / 120", view_model_get_ammo());

      bool open = true;
      igBegin("Ammo", &open, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
      igText(s);

      igEnd();
    }

    igPopStyleVar(3);
    igPopStyleColor(2);

    igPopFont();
  }

  // Respawn text display
  if (get_player_health() <= 0.0f)
  {
    const float ui_scale = screen_height / 720.0f;
    igPushFont(font_jura_med, FONT_SIZE * ui_scale * 0.5f);

    const ImU32 color = igGetColorU32_Vec4(foreground_color);
    const ImU32 shadow_color = igGetColorU32_Vec4(background_color);

    char text[32];
    if (player_get_respawn_cooldown() >= player_get_min_respawn_cooldown())
    {
      sprintf(text, "Press <FIRE> to respawn...");
    }
    else
    {
      const float time_remaining = player_get_min_respawn_cooldown() - player_get_respawn_cooldown();
      sprintf(text, "Respawn in %.1f seconds...", time_remaining);
    }

    ImVec2 size;
    igCalcTextSize(&size, text, NULL, false, 0.0f);

    // Shadow
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2){ screen_width / 2 - size.x / 2 - 1, screen_height / 2 + screen_height / 4 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2){ screen_width / 2 - size.x / 2 + 1, screen_height / 2 + screen_height / 4 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 - 1 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 + 1 },
                            shadow_color, text, NULL);

    // Actual text
    ImDrawList_AddText_Vec2(draw_list, (ImVec2){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 },
                            color, text, NULL);

    igPopFont();
  }

  update_hud_damage_indicator(draw_list, half_screen_width, half_screen_height, delta_time);

  if (preferences->show_player_info)
  {
    const ImU32 color = igGetColorU32_Vec4(foreground_color);

    {
      vec3 player_position;
      get_player_position(player_position);

      char s[128];
      sprintf(s, "Player position: %.2f, %.2f, %.2f", player_position[0], player_position[1], player_position[2]);
      ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, 100.0f }, color, s, NULL);
    }

    {
      vec3 player_velocity;
      get_player_velocity(player_velocity);

      char s[128];
      sprintf(s, "Player velocity: %.2f, %.2f, %.2f", player_velocity[0], player_velocity[1], player_velocity[2]);
      ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, 120.0f }, color, s, NULL);
    }

    {
      ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, 140.0f }, color,
                              get_player_grounded() ? "Grounded" : "In air", NULL);
    }

    {
      float yaw, pitch, roll;
      camera_get_yaw_pitch_roll(&yaw, &pitch, &roll);

      char s[128];
      sprintf(s, "Player angle: %.2f deg (yaw), %.2f deg (pitch), %.2f deg (roll)", glm_deg(yaw), glm_deg(pitch),
              glm_deg(roll));
      ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, 160.0f }, color, s, NULL);
    }
  }

  // Debug window
  if (debug_mode)
  {
    igSetNextWindowSizeConstraints((ImVec2){ screen_width / 4.0f, screen_height / 2.0f },
                                   (ImVec2){ screen_width, screen_height }, NULL, NULL);

    bool open = true;
    igBegin("Debug", &open, ImGuiWindowFlags_NoTitleBar);

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

      if (igTreeNode_Str("Primary directional"))
      {
        igColorEdit3("Color##Directional0", preferences->light_color0, ImGuiColorEditFlags_None);
        igSliderFloat("Intensity##Directional0", &preferences->light_intensity0, 0.0f, 1000.0f, "%f",
                      ImGuiSliderFlags_None);
        igSliderFloat3("Direction0", preferences->light_dir0, -1.0f, 1.0f, "%f", ImGuiSliderFlags_None);

        igTreePop();
      }

      if (igTreeNode_Str("Secondary directional"))
      {
        igColorEdit3("Color##Directional1", preferences->light_color1, ImGuiColorEditFlags_None);
        igSliderFloat("Intensity##Directional1", &preferences->light_intensity1, 0.0f, 1000.0f, "%f",
                      ImGuiSliderFlags_None);
        igSliderFloat3("Direction1", preferences->light_dir1, -1.0f, 1.0f, "%f", ImGuiSliderFlags_None);

        igTreePop();
      }
    }

    if (igCollapsingHeader_TreeNodeFlags("View model", ImGuiTreeNodeFlags_None))
    {
      igSliderFloat3("Position", preferences->view_model_position, -10.0f, 10.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Scale", &preferences->view_model_scale, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Field of view##ViewModel", &preferences->view_model_fov, 0.0f, 180.0f, "%f",
                    ImGuiSliderFlags_None);
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

    debug_window_focus =
      igIsWindowFocused(ImGuiFocusedFlags_AnyWindow) || igIsWindowHovered(ImGuiHoveredFlags_AnyWindow);

    igEnd();
  }

  /*bool yes = true;
  igShowDemoWindow(&yes);*/
}

void render_hud()
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}

void free_hud()
{
  free_hud_damage_indicator();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}