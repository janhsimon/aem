#include "gui.h"

#include "animation_mixer.h"
#include "display_state.h"
#include "main_menu.h"
#include "skeleton.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#define CIMGUI_USE_GLFW
#define CIMGUI_USE_OPENGL3
#include <cimgui/cimgui_impl.h>

// #define SHOW_DEMO_WINDOW

static ImGuiContext* context = NULL;
static ImGuiIO* io = NULL;

struct DisplayState* display_state;

void init_gui(struct GLFWwindow* window,
              struct DisplayState* display_state_,
              struct SceneState* scene_state,
              struct SkeletonState* skeleton_state,
              void (*file_open_callback)())
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

  display_state = display_state_;

  init_main_menu(display_state, scene_state, file_open_callback);
  init_skeleton(skeleton_state);
}

bool is_mouse_consumed()
{
  return io->WantCaptureMouse;
}

bool is_keyboard_consumed()
{
  return io->WantCaptureKeyboard;
}

void gui_on_new_model_loaded()
{
  animation_mixer_on_new_model();
  skeleton_on_new_model();
}

void update_gui(int screen_width, int screen_height)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  igNewFrame();

#ifdef SHOW_DEMO_WINDOW
  igShowDemoWindow(NULL);
#endif

  update_main_menu();
  update_animation_mixer(screen_width, screen_height);

  if (display_state->show_skeleton)
  {
    update_skeleton(screen_width, screen_height);
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
  destroy_skeleton();
  destroy_animation_mixer();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  igDestroyContext(context);
}