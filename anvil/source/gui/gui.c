#include "gui.h"

#include "grid.h"
#include "input.h"
#include "main_menu.h"
#include "tool/tool_state.h"
#include "viewport/viewport.h"

#include <util/util.h>

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

static GLuint icon_tex;

bool init_gui(struct GLFWwindow* window, void (*file_open_callback)())
{
  context = igCreateContext(NULL);

  io = igGetIO_ContextPtr(context);
  io->IniFilename = NULL;
  io->LogFilename = NULL;

  const char* glsl_version = "#version 330 core";
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
  {
    printf("Failed to initialize ImGui backend");
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init(glsl_version))
  {
    printf("Failed to initialize ImGui backend");
    return false;
  }

  init_main_menu(file_open_callback);

  if (!util_load_texture("textures/icons.png", UtilTextureWrapMode_ClampToEdge, &icon_tex))
  {
    return false;
  }

  return true;
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
  if (avail.x <= 0.0f || avail.y <= 0.0f)
  {
    igEndChild();
    return;
  }

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

static bool toolbar_tool(enum Tool tool,
                         const char* id,
                         const char* label,
                         ImTextureRef tex_ref,
                         uint32_t sprite_index,
                         uint32_t sprite_count,
                         float width)
{
  const ImVec2 text_size = igCalcTextSize(label, NULL, false, -1.0f);

  const float image_size = 32.0f;
  const float padding = 5.0f;
  const float spacing = 4.0f;
  const float frame_height = padding * 2.0f + image_size + spacing + text_size.y;

  // Colors for the complete button frame (selected tools stay visually pressed)
  const ImVec4 active = (ImVec4){ 0.28f, 0.28f, 0.28f, 1.0f };
  const ImVec4 hovered = (ImVec4){ 0.20f, 0.20f, 0.20f, 1.0f };
  const ImVec4 normal = (get_active_tool() == tool) ? active : (ImVec4){ 0.0f, 0.0f, 0.0f, 0.0f };

  igPushStyleColor_Vec4(ImGuiCol_Button, normal);
  igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, hovered);
  igPushStyleColor_Vec4(ImGuiCol_ButtonActive, active);

  // The button itself covers the entire row
  const bool clicked = igButton(id, (ImVec2){ width, frame_height });

  // Draw the image and text over the button
  ImDrawList* draw_list = igGetWindowDrawList();

  const ImU32 white = igGetColorU32_Vec4((ImVec4){ 1.0f, 1.0f, 1.0f, 1.0f });

  // Get the screen rectangle of the button
  const ImVec2 item_min = igGetItemRectMin();
  const ImVec2 item_max = igGetItemRectMax();
  const float item_center_x = (item_min.x + item_max.x) * 0.5f;

  // Image
  const float image_x = item_center_x - image_size * 0.5f;
  const float image_y = item_min.y + padding;
  {
    const ImVec2 image_min = (ImVec2){ image_x, image_y };
    const ImVec2 image_max = (ImVec2){ image_x + image_size, image_y + image_size };

    const ImVec2 uv0 = (ImVec2){ (float)sprite_index / (float)sprite_count, 0.0f };
    const ImVec2 uv1 = (ImVec2){ (float)(sprite_index + 1) / (float)sprite_count, 1.0f };

    ImDrawList_AddImage(draw_list, tex_ref, image_min, image_max, uv0, uv1, white);
  }

  // Text
  {
    const ImVec2 text_position = (ImVec2){ item_center_x - text_size.x * 0.5f, image_y + image_size + spacing };
    ImDrawList_AddText_Vec2(draw_list, text_position, white, label, NULL);
  }

  igPopStyleColor(3);

  return clicked;
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

  const float toolbar_width = 64.0f;
  const float right_width = 400.0f;
  const float bottombar_height = 30.0f;
  const float viewport_width = avail.x - toolbar_width - right_width - VIEWPORT_SPACING;
  const float viewport_height = avail.y - bottombar_height - VIEWPORT_SPACING;

  // Toolbar
  {
    ImTextureRef_c tex_ref;
    tex_ref._TexID = (ImTextureID)icon_tex;
    tex_ref._TexData = NULL;

    igBeginChild_Str("Toolbar", (ImVec2){ toolbar_width, viewport_height }, ImGuiChildFlags_None,
                     ImGuiWindowFlags_None);

    if (toolbar_tool(Tool_AddCube, "##CubeToolbarButton", "Cube", tex_ref, 0, 6, toolbar_width))
    {
      activate_tool(Tool_AddCube);
    }

    igSeparator();

    if (toolbar_tool(Tool_Scale, "##ScaleToolbarButton", "Scale", tex_ref, 1, 6, toolbar_width))
    {
      activate_tool(Tool_Scale);
    }

    if (toolbar_tool(Tool_Rotate, "##RotateToolbarButton", "Rotate", tex_ref, 2, 6, toolbar_width))
    {
      activate_tool(Tool_Rotate);
    }

    if (toolbar_tool(Tool_EditVertex, "##VertexToolbarButton", "Vertex", tex_ref, 3, 6, toolbar_width))
    {
      activate_tool(Tool_EditVertex);
    }

    igSeparator();

    if (toolbar_tool(Tool_ApplyTexture, "##TextureToolbarButton", "Texture", tex_ref, 4, 6, toolbar_width))
    {
      activate_tool(Tool_ApplyTexture);
    }

    igEndChild();
  }

  igSameLine(0, 0);

  // Viewport area
  {
    igBeginChild_Str("ViewportArea", (ImVec2){ viewport_width, viewport_height }, ImGuiChildFlags_None,
                     ImGuiWindowFlags_None);

    const float half_w = viewport_width * 0.5f;
    const float half_h = viewport_height * 0.5f;

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
    igBeginChild_Str("SidePanel", (ImVec2){ right_width, viewport_height }, ImGuiChildFlags_None,
                     ImGuiWindowFlags_None);

    igEndChild();
  }

  igPopStyleVar(2);

  // Bottom bar
  {
    igBeginChild_Str("BottomPanel", (ImVec2){ 0, bottombar_height }, ImGuiChildFlags_None, ImGuiWindowFlags_None);

    if (igBeginTable("BottomRow", 3, ImGuiTableFlags_None, (ImVec2){ 0, 0 }, 0.0f))
    {
      igTableNextColumn();
      igTableNextColumn();
      igTableNextColumn();

      int new_grid_size_index = get_grid_size_index();
      if (igSliderInt("##GridSize", &new_grid_size_index, 0, get_grid_size_count() - 1, get_grid_size_label(),
                      ImGuiSliderFlags_None))
      {
        grid_set_index(new_grid_size_index);
      }
      if (has_grid_size_tooltip() && igIsItemHovered(ImGuiHoveredFlags_None))
      {
        igBeginTooltip();
        igText(get_grid_size_tooltip());
        igEndTooltip();
      }

      igEndTable();
    }

    igEndChild();
  }

  igEnd();

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
  util_free_texture(icon_tex);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}