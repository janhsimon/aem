#include "gui.h"

#include "animation_state.h"
#include "camera.h"
#include "display_state.h"
#include "gui_skeleton.h"
#include "scene_state.h"

#include <aem/aem.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui_impl.h>

#include <math.h>

// #define SHOW_DEMO_WINDOW

#define PLAYBACK_BUTTON_WIDTH 50.0f

#define BIND_POSE_LABEL "Static (Bind Pose)"

static ImGuiContext* context = NULL;
static ImGuiIO* io = NULL;

static void (*file_open_callback)() = NULL;

struct AnimationState* animation_state;
struct DisplayState* display_state;
struct SceneState* scene_state;
struct SkeletonState* skeleton_state;

void init_gui(struct GLFWwindow* window,
              struct AnimationState* animation_state_,
              struct DisplayState* display_state_,
              struct SceneState* scene_state_,
              void (*file_open_callback_)())
{
  context = igCreateContext(NULL);

  io = igGetIO();
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

  animation_state = animation_state_;
  display_state = display_state_;
  scene_state = scene_state_;

  file_open_callback = file_open_callback_;
}

bool is_mouse_consumed()
{
  return io->WantCaptureMouse;
}

bool is_keyboard_consumed()
{
  return io->WantCaptureKeyboard;
}

void gui_on_new_model_loaded(struct SkeletonState* skeleton_state, struct AEMJoint* joints, uint32_t joint_count)
{
  init_gui_skeleton(skeleton_state, joints, joint_count);
}

void update_gui(int screen_width, int screen_height, char** animation_names, float animation_duration)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

#ifdef SHOW_DEMO_WINDOW
  igShowDemoWindow(NULL);
#endif

  // Main menu
  if (igBeginMainMenuBar())
  {
    if (igBeginMenu("File", true))
    {
      if (igMenuItem_Bool("Open...", "O", false, true))
      {
        if (file_open_callback)
        {
          file_open_callback();
        }
      }

      igEndMenu();
    }

    if (igBeginMenu("Render Mode", true))
    {
      if (igMenuItem_Bool("Full", "", display_state->render_mode == RenderMode_Full, true))
      {
        display_state->render_mode = RenderMode_Full;
      }

      igSeparator();

      if (igMenuItem_Bool("Vertex Position", "", display_state->render_mode == RenderMode_VertexPosition, true))
      {
        display_state->render_mode = RenderMode_VertexPosition;
      }
      if (igMenuItem_Bool("Vertex Normal", "", display_state->render_mode == RenderMode_VertexNormal, true))
      {
        display_state->render_mode = RenderMode_VertexNormal;
      }
      if (igMenuItem_Bool("Vertex Tangent", "", display_state->render_mode == RenderMode_VertexTangent, true))
      {
        display_state->render_mode = RenderMode_VertexTangent;
      }
      if (igMenuItem_Bool("Vertex Bitangent", "", display_state->render_mode == RenderMode_VertexBitangent, true))
      {
        display_state->render_mode = RenderMode_VertexBitangent;
      }
      if (igMenuItem_Bool("Vertex UV", "", display_state->render_mode == RenderMode_VertexUV, true))
      {
        display_state->render_mode = RenderMode_VertexUV;
      }

      igSeparator();

      if (igMenuItem_Bool("Base Color Texture", "", display_state->render_mode == RenderMode_TextureBaseColor, true))
      {
        display_state->render_mode = RenderMode_TextureBaseColor;
      }

      if (igMenuItem_Bool("Opacity Texture", "", display_state->render_mode == RenderMode_TextureOpacity, true))
      {
        display_state->render_mode = RenderMode_TextureOpacity;
      }

      if (igMenuItem_Bool("Normal Texture", "", display_state->render_mode == RenderMode_TextureNormal, true))
      {
        display_state->render_mode = RenderMode_TextureNormal;
      }

      if (igMenuItem_Bool("Roughness Texture", "", display_state->render_mode == RenderMode_TextureRoughness, true))
      {
        display_state->render_mode = RenderMode_TextureRoughness;
      }

      if (igMenuItem_Bool("Occlusion Texture", "", display_state->render_mode == RenderMode_TextureOcclusion, true))
      {
        display_state->render_mode = RenderMode_TextureOcclusion;
      }

      if (igMenuItem_Bool("Metalness Texture", "", display_state->render_mode == RenderMode_TextureMetalness, true))
      {
        display_state->render_mode = RenderMode_TextureMetalness;
      }

      if (igMenuItem_Bool("Emissive Texture", "", display_state->render_mode == RenderMode_TextureEmissive, true))
      {
        display_state->render_mode = RenderMode_TextureEmissive;
      }

      igSeparator();

      if (igMenuItem_Bool("Combined Normal", "", display_state->render_mode == RenderMode_CombinedNormal, true))
      {
        display_state->render_mode = RenderMode_CombinedNormal;
      }

      igEndMenu();
    }

    if (igBeginMenu("Animation", true))
    {
      // Loop
      if (igMenuItem_Bool("Loop", "L", animation_state->loop, true))
      {
        animation_state->loop = !animation_state->loop;
      }

      // Playback speed
      igSliderInt("##AnimationPlaybackSpeed", &animation_state->speed, 1, 500, "Playback Speed: %d%%", 0);

      igEndMenu();
    }

    if (igBeginMenu("View", true))
    {
      igMenuItem_BoolPtr("Render Transparent", "T", &display_state->render_transparent, true);

      igSeparator();

      // Scene scale
      igSliderInt("##SceneScale", &scene_state->scale, 1, 500, "Scene Scale: %d%%", 0);
      igSeparator();

      // Reset camera pivot
      if (igMenuItem_Bool("Reset Camera Pivot", "P", false, true))
      {
        reset_camera_pivot();
      }

      // Camera fov
      igSliderInt("##CameraFOV", &scene_state->camera_fov, 10, 160, "Camera FOV: %d deg", 0);

      // Auto-rotate camera
      if (igMenuItem_Bool("Auto-Rotate Camera", "R", scene_state->auto_rotate_camera, true))
      {
        scene_state->auto_rotate_camera = !scene_state->auto_rotate_camera;
      }

      igSliderInt("##AutoRotateCameraSpeed", &scene_state->auto_rotate_camera_speed, 1, 500, "Camera Speed: %d%%", 0);

      igSeparator();

      igColorEdit3("Background Color", scene_state->background_color, ImGuiColorEditFlags_NoInputs);

      igColorEdit3("Ambient Color", scene_state->ambient_color, ImGuiColorEditFlags_NoInputs);
      igSliderFloat("##AmbientIntensity", &scene_state->ambient_color[3], 0.0f, 5.0f, "Ambient Intensity: %.2f", 0);

      igColorEdit3("Light Color", scene_state->light_color, ImGuiColorEditFlags_NoInputs);
      igSliderFloat("##LightIntensity", &scene_state->light_color[3], 0.0f, 100.0f, "Light Intensity: %.2f", 0);

      igSeparator();

      // Show flags
      if (igMenuItem_Bool("Show GUI", "U", display_state->show_gui, true))
      {
        display_state->show_gui = !display_state->show_gui;
      }
      if (igMenuItem_Bool("Show Grid", "G", display_state->show_grid, true))
      {
        display_state->show_grid = !display_state->show_grid;
      }
      if (igMenuItem_Bool("Show Skeleton", "S", display_state->show_skeleton, true))
      {
        display_state->show_skeleton = !display_state->show_skeleton;
      }
      if (igMenuItem_Bool("Show Wireframe", "W", display_state->show_wireframe, true))
      {
        display_state->show_wireframe = !display_state->show_wireframe;
      }

      igEndMenu();
    }

    if (igBeginMenu("Postprocessing", true))
    {
      if (igMenuItem_Bool("Linear", "", display_state->postprocessing_mode == PostprocessingMode_Linear, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_Linear;
      }

      if (igMenuItem_Bool("sRGB", "", display_state->postprocessing_mode == PostprocessingMode_sRGB, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB;
      }

      if (igMenuItem_Bool("sRGB + Reinhard", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_Reinhard,
                          true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_Reinhard;
      }

      if (igMenuItem_Bool("sRGB + Reinhard X", "",
                          display_state->postprocessing_mode == PostprocessingMode_sRGB_ReinhardX, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_ReinhardX;
      }

      if (igMenuItem_Bool("sRGB + ACES", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_ACES, true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_ACES;
      }

      if (igMenuItem_Bool("sRGB + Filmic", "", display_state->postprocessing_mode == PostprocessingMode_sRGB_Filmic,
                          true))
      {
        display_state->postprocessing_mode = PostprocessingMode_sRGB_Filmic;
      }

      igEndMenu();
    }

    igEndMainMenuBar();
  }

  // Skeleton window
  if (display_state->show_skeleton)
  {
    update_gui_skeleton(screen_width, screen_height);
  }

  // Animation window
  {
    igSetNextWindowPos((struct ImVec2){ 0.0f, screen_height }, 0, (struct ImVec2){ 0.0f, 1.0f });
    igSetNextWindowSize((struct ImVec2){ screen_width, 0.0f }, 0);

    igBegin("##Animation", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    // Animation dropdown
    {
      igAlignTextToFramePadding();
      igText("Animation:");

      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(200.0f);
      if (igBeginCombo("##AnimationCombo",
                       animation_state->current_index < 0 ? BIND_POSE_LABEL :
                                                            animation_names[animation_state->current_index],
                       0))
      {
        if (igSelectable_Bool(BIND_POSE_LABEL, animation_state->current_index < 0, 0, (struct ImVec2){ 0, 0 }))
        {
          activate_animation(animation_state, -1);
        }

        for (unsigned int animation_index = 0; animation_index < animation_state->animation_count; ++animation_index)
        {
          if (igSelectable_Bool(animation_names[animation_index], animation_state->current_index == animation_index, 0,
                                (struct ImVec2){ 0, 0 }))
          {
            activate_animation(animation_state, animation_index);
          }
        }

        igEndCombo();
      }
    }

    igBeginDisabled(animation_state->current_index < 0);

    // Playback buttons
    {
      // Play/pause button
      igSameLine(0.0f, -1.0f);
      if (animation_state->playing)
      {
        if (igButton("Pause", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
        {
          animation_state->playing = false;
        }
      }
      else
      {
        if (igButton("Play", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
        {
          animation_state->playing = true;
        }
      }

      // Stop button
      igSameLine(0.0f, -1.0f);
      if (igButton("Stop", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
      {
        animation_state->playing = false;
        animation_state->time = 0.0f;
      }
    }

    // Timeline
    {
      const int playhead_m = (int)(animation_state->time / 60.0f);
      const int playhead_s = (int)animation_state->time;
      const int playhead_ms = (int)(fmod(animation_state->time, 1.0f) * 1000.0f);

      const int total_m = (int)(animation_duration / 60.0f);
      const int total_s = (int)animation_duration;
      const int total_ms = (int)(fmod(animation_duration, 1.0f) * 1000.0f);

      char buf[22];
      sprintf(buf, "%02d:%02d.%03d / %02d:%02d.%03d", playhead_m, playhead_s, playhead_ms, total_m, total_s, total_ms);

      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(-1.0f); // Stretch
      igSliderFloat("##AnimationTimeline", &animation_state->time, 0.0f, animation_duration, buf, 0);
    }

    igEndDisabled();

    igEnd();
  }

  // End the frame if there won't be rendering later
  if (!display_state->show_gui)
  {
    igEndFrame();
  }
}

void render_gui()
{
  igRender();
  ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
}

void destroy_gui()
{
  destroy_gui_skeleton();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}