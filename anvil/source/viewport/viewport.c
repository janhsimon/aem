#include "viewport.h"

#include "camera.h"
#include "framebuffer.h"
#include "grid.h"
#include "gui/gui.h"
#include "input.h"
#include "scene.h"
#include "tool/camera_tool.h"
#include "tool/move_tool_gizmo.h"
#include "tool/rotate_tool.h"
#include "tool/scale_tool.h"
#include "tool/select_tool.h"
#include "tool/select_tool_gizmo.h"
#include "tool/tool_state.h"

#include <cglm/affine.h>
#include <cglm/mat4.h>
#include <cglm/vec2.h>

#include <glad/gl.h>

#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f

static struct ToolState* tool_state = NULL;

struct Viewport
{
  enum ViewportType type;
  struct Camera* camera;
  struct ViewportFramebuffer* framebuffer;
  uint32_t x, y;
  uint32_t width, height;
  bool is_mouse_looking;
};

static vec2 mouse_look_anchor; // Only used for perspective viewports

bool make_viewport(struct Viewport** viewport,
                   enum ViewportType type,
                   uint32_t x,
                   uint32_t y,
                   uint32_t width,
                   uint32_t height,
                   struct ToolState* tool_state_)
{
  tool_state = tool_state_;

  *viewport = malloc(sizeof(struct Viewport));
  if (!(*viewport))
  {
    return false;
  }

  (*viewport)->type = type;

  if (!make_camera(&(*viewport)->camera))
  {
    return false;
  }

  camera_assign_viewport((*viewport)->camera, *viewport);

  (*viewport)->x = x;
  (*viewport)->y = y;
  (*viewport)->width = width;
  (*viewport)->height = height;
  (*viewport)->is_mouse_looking = false;

  if (!make_viewport_framebuffer(&(*viewport)->framebuffer, width, height))
  {
    return false;
  }

  return true;
}

void destroy_viewport(struct Viewport* viewport)
{
  destroy_viewport_framebuffer(viewport->framebuffer);
  destroy_camera(viewport->camera);
  free(viewport);
}

void viewport_on_mouse_button_pressed(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (viewport->type == ViewportType_Perspective)
  {
    if (button == MouseButton_Left)
    {
      // Start mouse look
      glm_vec3_copy(mouse_position, mouse_look_anchor);
      viewport->is_mouse_looking = true;
      input_use_mouse_look_cursor();
    }
  }
  else
  {
    bool event_consumed = false;

    if (tool_state->active_tool == Tool_Scale)
    {
      event_consumed = scale_tool_on_mouse_button_pressed(viewport, button, mouse_position);
    }
    else if (tool_state->active_tool == Tool_Rotate)
    {
      event_consumed = rotate_tool_on_mouse_button_pressed(viewport, button, mouse_position);
    }

    if (!event_consumed)
    {
      if (!select_tool_on_mouse_button_pressed(viewport, button, mouse_position))
      {
        camera_tool_on_mouse_button_pressed(viewport, button, mouse_position);
      }
    }
  }
}

void viewport_on_mouse_button_clicked(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (viewport->type != ViewportType_Perspective)
  {
    if (tool_state->active_tool == Tool_Scale)
    {
      scale_tool_on_mouse_button_clicked(viewport, button, mouse_position);
    }
    else if (tool_state->active_tool == Tool_Rotate)
    {
      rotate_tool_on_mouse_button_clicked(viewport, button, mouse_position);
    }
  }
  else if (viewport->is_mouse_looking)
  {
    viewport->is_mouse_looking = false;
    input_use_default_cursor();
  }
}

void viewport_on_mouse_moved(struct Viewport* viewport, vec2 mouse_position)
{
  if (viewport->type == ViewportType_Perspective)
  {
    return;
  }

  if (tool_state->active_tool == Tool_Scale)
  {
    if (!scale_tool_on_mouse_moved(viewport, mouse_position))
    {
      input_use_default_cursor();
    }
  }
  else if (tool_state->active_tool == Tool_Rotate)
  {
    if (!rotate_tool_on_mouse_moved(viewport, mouse_position))
    {
      input_use_default_cursor();
    }
  }
}

void viewport_on_mouse_dragged(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (viewport->type == ViewportType_Perspective)
  {
    // Mouse look
    if (button == MouseButton_Left)
    {
      vec2 delta;
      glm_vec2_sub(mouse_position, mouse_look_anchor, delta);
      glm_vec2_scale(delta, 0.01f, delta);

      camera_mouse_look_3d(viewport->camera, delta);

      glm_vec2_copy(mouse_position, mouse_look_anchor);
    }
  }
  else
  {
    bool event_consumed = false;

    if (tool_state->active_tool == Tool_Scale)
    {
      event_consumed = scale_tool_on_mouse_dragged(viewport, button, mouse_position);
    }
    else if (tool_state->active_tool == Tool_Rotate)
    {
      event_consumed = rotate_tool_on_mouse_dragged(viewport, button, mouse_position);
    }

    if (!event_consumed)
    {
      if (!select_tool_on_mouse_dragged(viewport, button, mouse_position))
      {
        camera_tool_on_mouse_dragged(viewport, button, mouse_position);
      }
    }
  }
}

void viewport_on_mouse_drag_finished(struct Viewport* viewport, enum MouseButton button, vec2 mouse_position)
{
  if (viewport->is_mouse_looking)
  {
    viewport->is_mouse_looking = false;
  }

  select_tool_on_mouse_drag_finished();

  input_use_default_cursor();
}

void viewport_on_mouse_scrolled(struct Viewport* viewport, vec2 mouse_position, float delta)
{
  if (viewport->type != ViewportType_Perspective)
  {
    camera_tool_on_mouse_scrolled(viewport, mouse_position, delta);
    scale_tool_reanchor(viewport, mouse_position);
  }
}

enum ViewportType viewport_get_type(const struct Viewport* viewport)
{
  return viewport->type;
}

const char* viewport_get_type_name(const struct Viewport* viewport)
{
  if (viewport->type == ViewportType_Perspective)
  {
    return "Perspective";
  }
  else if (viewport->type == ViewportType_Top)
  {
    return "Top";
  }
  else if (viewport->type == ViewportType_Front)
  {
    return "Front";
  }
  else if (viewport->type == ViewportType_Left)
  {
    return "Left";
  }

  return "";
}

void viewport_get_position(const struct Viewport* viewport, uint32_t* x, uint32_t* y)
{
  *x = viewport->x;
  *y = viewport->y;
}

void viewport_set_position(struct Viewport* viewport, uint32_t x, uint32_t y)
{
  viewport->x = x;
  viewport->y = y;
}

void viewport_get_resolution(const struct Viewport* viewport, uint32_t* width, uint32_t* height)
{
  *width = viewport->width;
  *height = viewport->height;
}
void viewport_set_resolution(struct Viewport* viewport, uint32_t width, uint32_t height)
{
  viewport->width = width;
  viewport->height = height;

  viewport_framebuffer_on_resize(viewport->framebuffer, width, height);
}

uint32_t viewport_get_framebuffer_texture(const struct Viewport* viewport)
{
  const uint32_t framebuffer_texture = viewport_framebuffer_get_texture(viewport->framebuffer);
  return framebuffer_texture;
}

struct Camera* viewport_get_camera(const struct Viewport* viewport)
{
  struct Camera* camera = viewport->camera;
  return camera;
}

bool viewport_is_mouse_looking(const struct Viewport* viewport)
{
  return viewport->is_mouse_looking;
}

void viewport_render(const struct Viewport* viewport)
{
  mat4 view_matrix, proj_matrix, viewproj_matrix;
  camera_calc_view_matrix(viewport->camera, view_matrix);
  camera_calc_proj_matrix(viewport->camera, proj_matrix);
  glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);

  viewport_framebuffer_start_rendering(viewport->framebuffer);

  glClearColor(BACKGROUND_COLOR, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const bool is_perspective_viewport = (viewport->type == ViewportType_Perspective);
  if (is_perspective_viewport)
  {
    // Draw scene
    {
      mat4 world_matrix = GLM_MAT4_IDENTITY_INIT;
      draw_scene(world_matrix, viewproj_matrix, is_perspective_viewport);
    }

    glDepthMask(GL_FALSE);

    // Draw grid
    {
      const bool fade_edges = true;
      draw_grid(viewproj_matrix, fade_edges);
    }

    glDepthMask(GL_TRUE);
  }
  else
  {
    glDisable(GL_DEPTH_TEST);

    // Draw grid
    {
      mat4 world_matrix, worldviewproj_matrix;
      glm_mat4_identity(world_matrix);
      if (viewport->type == ViewportType_Front)
      {
        glm_rotate_make(world_matrix, GLM_PI_2f, GLM_XUP);
      }
      else if (viewport->type == ViewportType_Left)
      {
        glm_rotate_make(world_matrix, GLM_PI_2f, GLM_ZUP);
      }

      glm_mat4_mul(viewproj_matrix, world_matrix, worldviewproj_matrix);

      // perspective_pipeline_start_rendering();

      const bool fade_edges = (viewport->type == ViewportType_Perspective);
      draw_grid(worldviewproj_matrix, fade_edges);
    }

    // Draw scene
    {
      mat4 world_matrix = GLM_MAT4_IDENTITY_INIT;
      draw_scene(world_matrix, viewproj_matrix, is_perspective_viewport);
    }

    // Draw gizmos
    {
      draw_select_tool_gizmo(viewport);
      draw_move_tool_gizmo(viewport);
    }

    glEnable(GL_DEPTH_TEST);
  }
}