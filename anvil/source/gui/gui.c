#include "gui.h"

#include "input.h"
#include "main_menu.h"
#include "viewport/viewport.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>
#include <cimgui/cimguizmo.h>

#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui_impl.h>

#include <math.h>

// #define SHOW_DEMO_WINDOW

#define VIEWPORT_SPACING 5       // Space between the viewport, horizontally and vertically
#define VIEWPORT_TITLE_OFFSET 5  // Distance for viewport title from top-left corner of the viewport
#define VIEWPORT_TITLE_PADDING 3 // Padding between the background of the viewport title and the text

static ImGuiContext* context = NULL;
static ImGuiIO* io = NULL;

static struct Viewport* hovered_viewport;

void init_gui(struct GLFWwindow* window, struct ToolState* tool_state, void (*file_open_callback)())
{
  context = igCreateContext(NULL);

  io = igGetIO_ContextPtr(context);
  io->IniFilename = NULL;
  io->LogFilename = NULL;

  const char* glsl_version = "#version 330 core";
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
  {
    printf("Failed to initialize ImGui backend");
    return;
  }

  if (!ImGui_ImplOpenGL3_Init(glsl_version))
  {
    printf("Failed to initialize ImGui backend");
    return;
  }

  init_main_menu(tool_state, file_open_callback);
}

// bool is_mouse_consumed()
//{
//   return io->WantCaptureMouse;
// }
//
// bool is_keyboard_consumed()
//{
//   return io->WantCaptureKeyboard;
// }
//
// bool is_mouse_over_guizmo()
//{
//   return ImGuizmo_IsOver_Nil();
// }

struct Viewport* gui_get_hovered_viewport()
{
  return hovered_viewport;
}

static void render_viewport_gui(struct Viewport* viewport, const ImVec2 size, const char* title)
{
  igBeginChild_Str(title, size, true, 0);

  const ImVec2 avail = igGetContentRegionAvail();

  viewport_set_resolution(viewport, (uint32_t)avail.x, (uint32_t)avail.y);

  ImTextureRef_c tex_ref;
  tex_ref._TexID = viewport_get_framebuffer_texture(viewport);
  tex_ref._TexData = NULL;

  igImage(tex_ref, avail, (ImVec2_c){ 0.0f, 0.0f }, (ImVec2_c){ 1.0f, 1.0f });

  const ImVec2 viewport_min = igGetItemRectMin();
  viewport_set_position(viewport, (uint32_t)viewport_min.x, (uint32_t)viewport_min.y);

  const ImU32 white = igGetColorU32_Vec4((ImVec4){ 1.0f, 1.0f, 1.0f, 1.0f });
  const ImU32 black = igGetColorU32_Vec4((ImVec4){ 0.0f, 0.0f, 0.0f, 1.0f });

  // Print the name of the viewport type
  {
    ImDrawList* draw = igGetWindowDrawList();

    const char* name = viewport_get_type_name(viewport);
    ImVec2 text_size = igCalcTextSize(name, NULL, false, 0.0f);

    const ImVec2 textbox_min = { viewport_min.x + VIEWPORT_TITLE_OFFSET, viewport_min.y + VIEWPORT_TITLE_OFFSET };
    const ImVec2 textbox_max = { textbox_min.x + text_size.x + VIEWPORT_TITLE_PADDING * 2,
                                 textbox_min.y + text_size.y + VIEWPORT_TITLE_PADDING * 2 };

    ImDrawList_AddRectFilled(draw, textbox_min, textbox_max, black, 0.0f, ImDrawFlags_None);

    const ImVec2 text_min = { textbox_min.x + VIEWPORT_TITLE_PADDING, textbox_min.y + VIEWPORT_TITLE_PADDING };
    ImDrawList_AddText_Vec2(draw, text_min, white, name, NULL);
  }

  if (viewport_is_mouse_looking(viewport))
  {
    ImDrawList* draw = igGetWindowDrawList();

    ImVec2 viewport_center;
    viewport_center.x = floorf(viewport_min.x + avail.x * 0.5f);
    viewport_center.y = floorf(viewport_min.y + avail.y * 0.5f);

    {
      ImDrawList_AddLine(draw, (ImVec2){ viewport_center.x - 10.0f, viewport_center.y },
                         (ImVec2){ viewport_center.x + 10.0f, viewport_center.y }, black, 3.0f);

      ImDrawList_AddLine(draw, (ImVec2){ viewport_center.x, viewport_center.y - 10.0f },
                         (ImVec2){ viewport_center.x, viewport_center.y + 10.0f }, black, 3.0f);
    }

    {
      ImDrawList_AddLine(draw, (ImVec2){ viewport_center.x - 10.0f, viewport_center.y },
                         (ImVec2){ viewport_center.x + 10.0f, viewport_center.y }, white, 1.0f);

      ImDrawList_AddLine(draw, (ImVec2){ viewport_center.x, viewport_center.y - 10.0f },
                         (ImVec2){ viewport_center.x, viewport_center.y + 10.0f }, white, 1.0f);
    }
  }

  if (igIsWindowHovered(ImGuiHoveredFlags_None))
  {
    hovered_viewport = viewport;
  }

  igEndChild();
}

void update_gui(struct Viewport* viewports[4])
{
  hovered_viewport = NULL;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

  ImGuizmo_BeginFrame();
  ImGuizmo_SetRect(0.0f, 0.0f, io->DisplaySize.x, io->DisplaySize.y);

  update_main_menu();

  ImGuiViewport* vp = igGetMainViewport();

  igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){ 0, 0 });
  igPushStyleVar_Vec2(ImGuiStyleVar_ItemSpacing, (ImVec2){ 0, VIEWPORT_SPACING });

  igSetNextWindowPos(vp->WorkPos, ImGuiCond_Always, (ImVec2){ 0, 0 });
  igSetNextWindowSize(vp->WorkSize, ImGuiCond_Always);
  igBegin("Editor", NULL,
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse);

  const ImVec2 avail = igGetContentRegionAvail();

  const float right_width = 400.0f;
  const float viewport_width = avail.x - right_width;

  // Viewport area
  {
    igBeginChild_Str("ViewportArea", (ImVec2){ viewport_width, avail.y }, false, 0);

    const float half_w = viewport_width * 0.5f;
    const float half_h = avail.y * 0.5f;

    render_viewport_gui(viewports[0], (ImVec2){ half_w, half_h }, "TopLeftViewport");
    igSameLine(0, VIEWPORT_SPACING);
    render_viewport_gui(viewports[1], (ImVec2){ 0, half_h }, "TopRightViewport");

    render_viewport_gui(viewports[2], (ImVec2){ half_w, 0 }, "BottomLeftViewport");
    igSameLine(0, VIEWPORT_SPACING);
    render_viewport_gui(viewports[3], (ImVec2){ 0, 0 }, "BottomRightViewport");

    igEndChild();
  }

  igSameLine(0, VIEWPORT_SPACING);

  // Side panel
  {
    igBeginChild_Str("SidePanel", (ImVec2){ 0, 0 }, true, 0);

    igEndChild();
  }

  igEnd();

  igPopStyleVar(2);

#ifdef SHOW_DEMO_WINDOW
  igShowDemoWindow(NULL);
#endif
}

void render_gui()
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}

void destroy_gui()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}