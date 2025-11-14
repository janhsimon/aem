#include "hud.h"

#include "debug_renderer.h"
#include "preferences.h"
#include "view_model.h"
#include "window.h"

#include <cglm/vec3.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui_impl.h>

static ImGuiContext* context = NULL;
static ImGuiIO* io = NULL;

bool load_hud()
{
  context = igCreateContext(NULL);

  io = igGetIO_ContextPtr(context);
  io->IniFilename = NULL;
  io->LogFilename = NULL;

  const char* glsl_version = "#version 330 core";
  if (!ImGui_ImplGlfw_InitForOpenGL(get_window(), true))
  {
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init(glsl_version))
  {
    return false;
  }

  return true;
}

void update_hud(uint32_t screen_width,
                uint32_t screen_height,
                bool debug_mode,
                float player_speed,
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

  if (preferences->show_player_move_speed)
  {
    char s[64];
    sprintf(s, "Movement speed: %f", player_speed);

    const ImU32 color = igGetColorU32_Vec4(foreground_color);
    ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, screen_height - 100.0f }, color, s, NULL);
  }

  // Debug window
  if (debug_mode)
  {
    bool open = true;
    igBegin("Debug", &open, ImGuiWindowFlags_NoTitleBar);

    if (igCollapsingHeader_TreeNodeFlags("Debugging", ImGuiTreeNodeFlags_None))
    {
      igCheckbox("Debug render", &preferences->debug_render);
      if (igButton("Clear lines", (ImVec2){ 0.0f, 0.0f }))
      {
        clear_debug_lines();
      }

      igCheckbox("Show player move speed", &preferences->show_player_move_speed);
    }

    if (igCollapsingHeader_TreeNodeFlags("Camera", ImGuiTreeNodeFlags_None))
    {
      igSliderFloat("Field of view##Camera", &preferences->camera_fov, 0.0f, 180.0f, "%f", ImGuiSliderFlags_None);
      igColorEdit3("Background color##Camera", preferences->camera_background_color, ImGuiColorEditFlags_None);
    }

    if (igCollapsingHeader_TreeNodeFlags("Ambient lighting", ImGuiTreeNodeFlags_None))
    {
      igColorEdit3("Color##Ambient", preferences->ambient_color, ImGuiColorEditFlags_None);
      igSliderFloat("Intensity##Ambient", &preferences->ambient_intensity, 0.0f, 1.0f, "%f", ImGuiSliderFlags_None);
    }

    if (igCollapsingHeader_TreeNodeFlags("Directional lighting", ImGuiTreeNodeFlags_None))
    {
      igColorEdit3("Color##Directional", preferences->light_color, ImGuiColorEditFlags_None);
      igSliderFloat("Intensity##Directional", &preferences->light_intensity, 0.0f, 1000.0f, "%f",
                    ImGuiSliderFlags_None);
      igSliderFloat3("Direction", preferences->light_dir, -1.0f, 1.0f, "%f", ImGuiSliderFlags_None);
    }

    if (igCollapsingHeader_TreeNodeFlags("View model", ImGuiTreeNodeFlags_None))
    {
      igSliderFloat3("Position", preferences->view_model_position, -10.0f, 10.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Scale", &preferences->view_model_scale, 0.0f, 100.0f, "%f", ImGuiSliderFlags_None);
      igSliderFloat("Field of view##ViewModel", &preferences->view_model_fov, 0.0f, 180.0f, "%f",
                    ImGuiSliderFlags_None);
    }

    if (igCollapsingHeader_TreeNodeFlags("HUD", ImGuiTreeNodeFlags_None))
    {
      igColorEdit4("Foreground color##HUD", preferences->hud_foreground_color, ImGuiColorEditFlags_None);
      igColorEdit4("Background color##HUD", preferences->hud_background_color, ImGuiColorEditFlags_None);
    }

    igEnd();
  }

  /*
  bool yes = true;
  igShowDemoWindow(&yes);
  */
}

void render_hud()
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}

void free_hud()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}