#include "hud.h"

#include "window.h"

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

void update_hud(uint32_t screen_width, uint32_t screen_height)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

  // Crosshair
  {
    ImDrawList* draw_list = igGetForegroundDrawList_WindowPtr(context->CurrentWindow);

    const ImU32 color = igGetColorU32_Vec4((ImVec4){ 0.0f, 1.0f, 0.0f, 1.0f });

    const float half_screen_width = (float)(screen_width / 2);
    const float half_screen_height = (float)(screen_height / 2);

    const float gap_size = 6.0f;
    const float half_gap_size = gap_size / 2.0f;
    const float line_size = 8.0f;

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