#include "main_menu.h"

#include "grid.h"
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

    if (igBeginMenu("Texture", true))
    {
      const bool texture_lock = get_texture_lock();
      if (igMenuItem_Bool("Lock texture", "L", texture_lock, true))
      {
        set_texture_lock(!texture_lock);
      }

      igEndMenu();
    }

    if (igBeginMenu("View", true))
    {
      const bool render_3d_grid = get_render_3d_grid();
      if (igMenuItem_Bool("Show 3D grid", "G", render_3d_grid, true))
      {
        set_render_3d_grid(!render_3d_grid);
      }

      igEndMenu();
    }

    igEndMainMenuBar();
  }
}