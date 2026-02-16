#include "texture_window.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#include <cglm/util.h>

bool begin_texture_window(const char* title,
                          bool menu_bar,
                          unsigned int window_index,
                          uint32_t screen_width,
                          uint32_t screen_height)
{
  igSetNextWindowPos((ImVec2){ screen_width - screen_width / 4, (screen_height / 4) * window_index }, ImGuiCond_Once,
                     (ImVec2){ 0.0f, 0.0f });

  igSetNextWindowSizeConstraints((ImVec2){ screen_width / 4, screen_height / 4 },
                                 (ImVec2){ screen_width, screen_height }, NULL, NULL);

  bool open = true;
  return igBegin(title, &open, menu_bar ? ImGuiWindowFlags_MenuBar : ImGuiWindowFlags_None);
}

void add_texture_window_image(unsigned int texture_index, uint32_t screen_width, uint32_t screen_height)
{
  // Get available space inside the window
  ImVec2 avail;
  igGetContentRegionAvail(&avail);

  // Compute scale to fit while preserving aspect ratio
  const float scale_x = avail.x / screen_width;
  const float scale_y = avail.y / screen_height;
  const float scale = glm_max(scale_x < scale_y ? scale_x : scale_y, 0.01f); // Prevent negative or zero scale

  const float draw_width = screen_width * scale;
  const float draw_height = screen_height * scale;

  // Center the image
  {
    const float offset_x = (avail.x - draw_width) * 0.5f;
    const float offset_y = (avail.y - draw_height) * 0.5f;
    igSetCursorPosX(igGetCursorPosX() + glm_max(offset_x, 0.0f));
    igSetCursorPosY(igGetCursorPosY() + glm_max(offset_y, 0.0f));
  }

  // Draw the texture
  ImTextureRef ref;
  ref._TexData = NULL;
  ref._TexID = texture_index;
  igImage(ref, (ImVec2){ draw_width, draw_height }, (ImVec2){ 0.0f, 1.0f }, (ImVec2){ 1.0f, 0.0f });
}

void end_texture_window()
{
  igEnd();
}