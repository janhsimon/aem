#include "main_menu.h"

#include "tool/tool_state.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static struct ToolState* tool_state = NULL;

static void (*file_open_callback)() = NULL;

void init_main_menu(struct ToolState* tool_state_, void (*file_open_callback_)())
{
  tool_state = tool_state_;

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

    if (igBeginMenu("Tool", true))
    {
      if (igMenuItem_Bool("Scale", "T", tool_state->active_tool == Tool_Scale, true))
      {
        tool_state->active_tool = Tool_Scale;
      }

      if (igMenuItem_Bool("Rotate", "R", tool_state->active_tool == Tool_Rotate, true))
      {
        tool_state->active_tool = Tool_Rotate;
      }

      if (igMenuItem_Bool("Vertex edit", "V", tool_state->active_tool == Tool_VertexEdit, true))
      {
        tool_state->active_tool = Tool_VertexEdit;
      }

      igEndMenu();
    }

    if (igBeginMenu("Texture", true))
    {
      if (igMenuItem_Bool("Lock texture", "L", tool_state->texture_lock, true))
      {
        tool_state->texture_lock = !tool_state->texture_lock;
      }

      igEndMenu();
    }

    igEndMainMenuBar();
  }
}