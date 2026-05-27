#include "hud.h"

#include "camera.h"
#include "debug_window.h"
#include "hud_damage_indicator.h"
#include "player/player.h"
#include "player/view_model.h"
#include "preferences.h"
#include "renderer/bloom_pass/bloom_framebuffer.h"
#include "renderer/debug_texture_pass/debug_texture_framebuffer.h"
#include "renderer/forward_pass/forward_framebuffer.h"
#include "renderer/ssao_pass/ssao_framebuffer.h"
#include "texture_window.h"
#include "window.h"

#include <cglm/util.h>
#include <cglm/vec2.h>

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

static void draw_crosshair_rect(ImDrawList* draw_list,
                                float from_x,
                                float from_y,
                                float width,
                                float height,
                                ImU32 background_color,
                                ImU32 foreground_color,
                                bool outline)
{
  const float to_x = from_x + width;
  const float to_y = from_y + height;

  // Background outline
  if (outline)
  {
    ImDrawList_AddRect(draw_list, (ImVec2){ from_x - 1.0f, from_y - 1.0f }, (ImVec2){ to_x + 1.0f, to_y + 1.0f },
                       background_color, 0.0f, ImDrawFlags_None, 1.0f);
  }

  // Foreground fill
  ImDrawList_AddRectFilled(draw_list, (ImVec2){ from_x, from_y }, (ImVec2){ to_x, to_y }, foreground_color, 0.0f,
                           ImDrawFlags_None);
}

void vec4_to_color(vec4 input, ImVec4 *output)
{
  output->x = input[0];
  output->y = input[1];
  output->z = input[2];
  output->w = input[3];
}

static float calc_half_screen(uint32_t size)
{
  // Even
  if (size % 2 == 0)
  {
    return (float)(size / 2 - 1);
  }

  // Odd
  return floorf((float)size / 2.0f);
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

  ImVec4 foreground_color, background_color, crosshair_outline_color;
  vec4_to_color(preferences->hud_foreground_color, &foreground_color);
  vec4_to_color(preferences->hud_background_color, &background_color);
  vec4_to_color(preferences->hud_crosshair_outline_color, &crosshair_outline_color);

  const float half_screen_width = calc_half_screen(screen_width);
  const float half_screen_height = calc_half_screen(screen_height);

  // Crosshair
  if (get_player_health() > 0.0f)
  {
    // Expand the crosshair to visualize movement spread
    float movement_spread;
    {
      movement_spread = glm_clamp(get_player_movement_spread(), (float)preferences->hud_crosshair_spread_min,
                                  (float)preferences->hud_crosshair_spread_max);
      movement_spread -= (float)preferences->hud_crosshair_spread_min;
      movement_spread /= ((float)preferences->hud_crosshair_spread_max - (float)preferences->hud_crosshair_spread_min);
      movement_spread =
        glm_lerp((float)preferences->hud_crosshair_gap_min, (float)preferences->hud_crosshair_gap_max, movement_spread);
    }

    // Also expand the crosshair when firing
    float fire_expand = 0.0f;
    if (view_model_get_ammo() >= 0)
    {
      fire_expand = view_model_get_normalized_shot_cooldown(preferences);
      fire_expand *= (float)preferences->hud_crosshair_fire_expand;
    }

    const float gap_size = movement_spread + fire_expand;
    const float line_size = preferences->hud_crosshair_length;

    const ImU32 crosshair_color = igGetColorU32_Vec4(foreground_color);
    const ImU32 outline_color = igGetColorU32_Vec4(crosshair_outline_color);

    const float thickness = (float)preferences->hud_crosshair_thickness;
    const float half_thickness = floorf(thickness / 2.0f);

    float correction = 0.0f;
    if (((uint32_t)thickness) % 2 == 1)
    {
      correction = 1.0f;
    }

    const bool outline = preferences->hud_crosshair_outline;

    // Lines
    if (preferences->hud_crosshair_lines)
    {
      // Left
      draw_crosshair_rect(draw_list, half_screen_width - gap_size - line_size, half_screen_height - half_thickness,
                          line_size, thickness, outline_color, crosshair_color, outline);

      // Right
      draw_crosshair_rect(draw_list, half_screen_width + gap_size + correction, half_screen_height - half_thickness,
                          line_size, thickness, outline_color, crosshair_color, outline);

      // Top
      draw_crosshair_rect(draw_list, half_screen_width - half_thickness, half_screen_height - gap_size - line_size,
                          thickness, line_size, outline_color, crosshair_color, outline);

      // Bottom
      draw_crosshair_rect(draw_list, half_screen_width - half_thickness, half_screen_height + gap_size + correction,
                          thickness, line_size, outline_color, crosshair_color, outline);
    }

    // Dot
    if (preferences->hud_crosshair_dot)
    {
      draw_crosshair_rect(draw_list, half_screen_width - half_thickness, half_screen_height - half_thickness, thickness,
                          thickness, outline_color, crosshair_color, outline);
    }

    // Center pixel for calibration
    /*const ImU32 red = igGetColorU32_Vec4((ImVec4){ 1.0f, 0.0f, 0.0f, 1.0f });
    ImDrawList_AddRectFilled(draw_list, (ImVec2){ half_screen_width, half_screen_height },
                             (ImVec2){ half_screen_width + 1, half_screen_height + 1 }, red, 0.0f, ImDrawFlags_None);*/
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

    const ImVec2_c size = igCalcTextSize(text, NULL, false, 0.0f);

    // Shadow
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2_c){ screen_width / 2 - size.x / 2 - 1, screen_height / 2 + screen_height / 4 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2_c){ screen_width / 2 - size.x / 2 + 1, screen_height / 2 + screen_height / 4 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2_c){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 - 1 },
                            shadow_color, text, NULL);
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2_c){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 + 1 },
                            shadow_color, text, NULL);

    // Actual text
    ImDrawList_AddText_Vec2(draw_list,
                            (ImVec2_c){ screen_width / 2 - size.x / 2, screen_height / 2 + screen_height / 4 }, color,
                            text, NULL);

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

      vec2 run = { player_velocity[0], player_velocity[2] };
      const float run_speed = glm_vec2_norm(run) * 1000.0f;

      char s[128];
      sprintf(s, "Player velocity: %.2f, %.2f, %.2f (run speed: %.2f)", player_velocity[0], player_velocity[1],
              player_velocity[2], run_speed);
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

    {
      char s[128];
      sprintf(s, "Frame time: %d ms\tFPS: %d", (int)(delta_time * 1000.0f), (int)(1.0f / delta_time));
      ImDrawList_AddText_Vec2(draw_list, (ImVec2){ 100.0f, 180.0f }, color, s, NULL);
    }
  }

  if (debug_mode)
  {
    update_debug_window(preferences, screen_width, screen_height);
  }

  // Texture windows
  {
    if (get_show_shadow_map_window())
    {
      if (begin_texture_window("Shadow map", true, 0, screen_width, screen_height))
      {
        static int shown_cascade_index = 0;

        if (igBeginMenuBar())
        {
          if (igBeginMenu("Cascade", true))
          {
            for (int cascade_index = 0; cascade_index < 4; ++cascade_index)
            {
              char title[32];
              const uint32_t texture_size = preferences->shadow_mapping_cascade_texture_sizes[cascade_index];
              sprintf(title, "Cascade #%d [%upx x %upx]", cascade_index, texture_size, texture_size);

              const bool selected = (shown_cascade_index == cascade_index);
              if (igMenuItemEx(title, NULL, NULL, selected, true))
              {
                shown_cascade_index = cascade_index;
              }
            }

            igEndMenu();
          }

          igEndMenuBar();

          add_texture_window_image(debug_texture_framebuffer_get_shadow_map(shown_cascade_index), 1024, 1024);
        }
      }

      end_texture_window();
    }

    if (get_show_view_space_normals_window())
    {
      if (begin_texture_window("View-space normals", false, 1, screen_width, screen_height))
      {
        add_texture_window_image(forward_framebuffer_get_view_space_normals_texture(), screen_width, screen_height);
      }

      end_texture_window();
    }

    if (get_show_ssao_window())
    {
      if (begin_texture_window("SSAO", false, 2, screen_width, screen_height))
      {
        add_texture_window_image(ssao_framebuffer_get_texture(0), screen_width, screen_height);
      }

      end_texture_window();
    }

    if (get_show_bloom_window())
    {
      if (begin_texture_window("Bloom", true, 3, screen_width, screen_height))
      {
        static int shown_texture_index = 0;
        static enum BloomFramebufferPhase shown_texture_phase = BloomFramebufferPhase_Downsample;

        if (igBeginMenuBar())
        {
          if (igBeginMenu("Texture", true))
          {
            const int downsample_texture_count = bloom_framebuffer_get_texture_count(BloomFramebufferPhase_Downsample);
            const int upsample_texture_count = bloom_framebuffer_get_texture_count(BloomFramebufferPhase_Upsample);

            // Downsample
            for (int texture_index = 0; texture_index < downsample_texture_count; ++texture_index)
            {
              uint32_t w, h;
              bloom_framebuffer_get_texture_resolution(texture_index, &w, &h);

              char title[32];
              sprintf(title, "Downsample #%d [%upx x %upx]", texture_index, w, h);

              const bool selected =
                (shown_texture_index == texture_index) && (shown_texture_phase == BloomFramebufferPhase_Downsample);
              if (igMenuItemEx(title, NULL, NULL, selected, true))
              {
                shown_texture_index = texture_index;
                shown_texture_phase = BloomFramebufferPhase_Downsample;
              }
            }

            // Upsample
            for (int texture_index = 0; texture_index < upsample_texture_count; ++texture_index)
            {
              const int flipped_index = upsample_texture_count - texture_index - 1;

              uint32_t w, h;
              bloom_framebuffer_get_texture_resolution(flipped_index, &w, &h);

              char title[32];
              sprintf(title, "Upsample #%d [%u x %u]", flipped_index, w, h);

              const bool selected =
                (shown_texture_index == flipped_index) && (shown_texture_phase == BloomFramebufferPhase_Upsample);
              if (igMenuItemEx(title, NULL, NULL, selected, true))
              {
                shown_texture_index = flipped_index;
                shown_texture_phase = BloomFramebufferPhase_Upsample;
              }
            }

            igEndMenu();
          }

          igEndMenuBar();

          add_texture_window_image(bloom_framebuffer_get_texture(shown_texture_index, shown_texture_phase),
                                   screen_width, screen_height);
        }
      }

      end_texture_window();
    }
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
  free_hud_damage_indicator();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}