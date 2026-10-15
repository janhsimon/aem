#include "main_menu.h"

#include "tool/tool_state.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

static void (*file_open_callback)() = NULL;

void init_main_menu(void (*file_open_callback_)())
{
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

    /*if (igBeginMenu("Tool", true))
    {
      if (igMenuItem_Bool("Scale", "T", tool_state->active_tool == Tool_Scale, true))
      {
        tool_state->active_tool = Tool_Scale;
      }

      if (igMenuItem_Bool("Rotate", "R", tool_state->active_tool == Tool_Rotate, true))
      {
        tool_state->active_tool = Tool_Rotate;
      }

      if (igMenuItem_Bool("Vertex", "V", tool_state->active_tool == Tool_Vertex, true))
      {
        tool_state->active_tool = Tool_Vertex;
      }

      igEndMenu();
    }*/

    if (igBeginMenu("Texture", true))
    {
      const bool texture_lock = get_texture_lock();
      if (igMenuItem_Bool("Lock texture", "L", texture_lock, true))
      {
        set_texture_lock(!texture_lock);
      }

      igEndMenu();
    }

    igEndMainMenuBar();
  }
}