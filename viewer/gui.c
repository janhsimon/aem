#include "gui.h"

#include "animation_state.h"
#include "display_state.h"
#include "scene_state.h"

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
      // Scene scale
      igSliderInt("##SceneScale", &scene_state->scale, 1, 500, "Scene Scale: %d%%", 0);
      igSeparator();

      // Auto-rotate camera
      if (igMenuItem_Bool("Auto-Rotate Camera", "R", scene_state->auto_rotate_camera, true))
      {
        scene_state->auto_rotate_camera = !scene_state->auto_rotate_camera;
      }

      igSliderInt("##AutoRotateCameraSpeed", &scene_state->auto_rotate_camera_speed, 1, 500, "Camera Speed: %d%%", 0);
      igSeparator();

      // Show flags
      if (igMenuItem_Bool("Show GUI", "U", display_state->gui, true))
      {
        display_state->gui = !display_state->gui;
      }
      if (igMenuItem_Bool("Show Grid", "G", display_state->grid, true))
      {
        display_state->grid = !display_state->grid;
      }
      if (igMenuItem_Bool("Show Skeleton", "S", display_state->skeleton, true))
      {
        display_state->skeleton = !display_state->skeleton;
      }

      igEndMenu();
    }

    igEndMainMenuBar();
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
  }

  igEnd();
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