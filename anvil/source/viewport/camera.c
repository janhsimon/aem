#include "camera.h"

#include "input.h"
#include "viewport/viewport.h"

#include <cglm/affine.h>
#include <cglm/cam.h>
#include <cglm/vec2.h>

#include <assert.h>

#define PITCH_CLAMP 1.5533f // 89 deg in rad

struct Camera
{
  struct Viewport* viewport;
  mat4 transform;
  float near, far;

  union
  {
    struct
    {
      float zoom;
    } orthographic;
    struct
    {
      float yaw, pitch;
      float fov;
    } perspective;
  };
};

bool make_camera(struct Camera** camera)
{
  *camera = malloc(sizeof(struct Camera));
  if (!(*camera))
  {
    return false;
  }

  return true;
}

void destroy_camera(struct Camera* camera)
{
  free(camera);
}

static void yaw_pitch_to_transform_3d(struct Camera* camera)
{
  // Forward vector from yaw and pitch
  vec3 forward = { cosf(camera->perspective.yaw) * cosf(camera->perspective.pitch), sinf(camera->perspective.pitch),
                   sinf(camera->perspective.yaw) * cosf(camera->perspective.pitch) };
  glm_vec3_normalize(forward);

  // Right vector
  vec3 right;
  glm_vec3_cross(GLM_YUP, forward, right);
  glm_vec3_normalize(right);

  // Up vector
  vec3 up;
  glm_vec3_cross(forward, right, up);

  glm_vec3_copy(right, camera->transform[0]);
  glm_vec3_copy(up, camera->transform[1]);
  glm_vec3_copy(forward, camera->transform[2]);
}

void camera_assign_viewport(struct Camera* camera, struct Viewport* viewport)
{
  camera->viewport = viewport;

  camera->near = 0.01f;
  camera->far = 1000.0f;

  {
    const enum ViewportType viewport_type = viewport_get_type(viewport);
    if (viewport_type == ViewportType_Perspective)
    {
      camera->perspective.yaw = GLM_PI_4f, camera->perspective.pitch = 0.0f;
      camera->perspective.fov = 100.0f;

      glm_mat4_identity(camera->transform);
      yaw_pitch_to_transform_3d(camera);
      glm_vec3_copy((vec3){ 50.0f, 10.0f, 50.0f }, camera->transform[3]);
    }
    else
    {
      camera->orthographic.zoom = 10.0f;

      vec3 eye, center, up;
      glm_vec3_copy(GLM_VEC3_ZERO, center);
      glm_vec3_copy(GLM_YUP, up);

      if (viewport_type == ViewportType_Top)
      {
        glm_vec3_copy((vec3){ 0.0f, 1.0f, 0.0f }, eye);
        glm_vec3_copy(GLM_ZUP, up); // Override the up axis
      }
      else if (viewport_type == ViewportType_Front)
      {
        glm_vec3_copy((vec3){ 0.0f, 0.0f, -1.0f }, eye);
      }
      else if (viewport_type == ViewportType_Left)
      {
        glm_vec3_copy((vec3){ -1.0f, 0.0f, 0.0f }, eye);
      }

      glm_vec3_scale(eye, 100.0f, eye);

      glm_lookat(eye, center, up, camera->transform);     // Produce a view matrix
      glm_mat4_inv(camera->transform, camera->transform); // Convert view to transform matrix
    }
  }
}

void camera_screen_to_world_2d(struct Camera* camera, vec2 in_screen, vec3 out_world)
{
  assert(viewport_get_type(camera->viewport) != ViewportType_Perspective);

  vec2 viewport_pos;
  {
    uint32_t x, y;
    viewport_get_position(camera->viewport, &x, &y);
    glm_vec2_copy((vec2){ (float)x, (float)y }, viewport_pos);
  }

  vec2 d;
  glm_vec2_sub(in_screen, viewport_pos, d);

  vec2 half_viewport_size;
  {
    uint32_t width, height;
    viewport_get_resolution(camera->viewport, &width, &height);
    glm_vec2_scale((vec2){ (float)width, (float)height }, 0.5f, half_viewport_size);
  }

  //vec2 d;
  glm_vec2_sub(d, half_viewport_size, d);

  const float world_per_pixel = camera->orthographic.zoom * 0.01f;
  glm_vec2_scale(d, world_per_pixel, d);

  vec3 offset_x;
  glm_vec3_scale(camera->transform[0], d[0], offset_x);

  vec3 offset_y;
  glm_vec3_scale(camera->transform[1], -d[1], offset_y);

  glm_vec3_add(camera->transform[3], camera->transform[2], out_world);
  glm_vec3_add(out_world, offset_x, out_world);
  glm_vec3_add(out_world, offset_y, out_world);
}

void camera_world_to_clip(struct Camera* camera, vec3 in_world, vec2 out_clip)
{
  vec4 clip = { in_world[0], in_world[1], in_world[2], 1.0f };

  mat4 viewproj;
  {
    mat4 view, proj;
    camera_calc_view_matrix(camera, view);
    camera_calc_proj_matrix(camera, proj);
    glm_mat4_mul(proj, view, viewproj);
  }

  glm_mat4_mulv(viewproj, clip, clip);

  glm_vec2_copy(clip, out_clip);
}

void camera_world_to_screen(struct Camera* camera, vec3 in_world, vec2 out_screen)
{
  camera_world_to_clip(camera, in_world, out_screen);

  vec2 viewport_size;
  {
    uint32_t width, height;
    viewport_get_resolution(camera->viewport, &width, &height);
    glm_vec2_copy((vec2){ (float)width, (float)height }, viewport_size);
  }

  out_screen[0] = (out_screen[0] * 0.5f + 0.5f) * viewport_size[0];
  out_screen[1] = (out_screen[1] * 0.5f + 0.5f) * viewport_size[1];
}

void camera_zoom_2d(struct Camera* camera, vec2 mouse_pos, float delta)
{
  assert(viewport_get_type(camera->viewport) != ViewportType_Perspective);

  vec3 before; // The mouse position in world space before the zoom
  camera_screen_to_world_2d(camera, mouse_pos, before);

  // Perform the actual zoom
  if (delta < 0.0f)
  {
    camera->orthographic.zoom *= 0.9f;
  }
  else if (delta > 0.0f)
  {
    camera->orthographic.zoom *= 1.1f;
  }

  // Clamp the zoom to min and max values
  camera->orthographic.zoom = glm_clamp(camera->orthographic.zoom, 0.01f, 1000.0f);

  vec3 after; // The mouse position in world space after the zoom
  camera_screen_to_world_2d(camera, mouse_pos, after);

  // Pan to achieve zoom into or out of mouse position
  vec3 correction;
  glm_vec3_sub(before, after, correction);
  glm_vec3_add(camera->transform[3], correction, camera->transform[3]);
}

void camera_pan_2d(struct Camera* camera, vec3 delta)
{
  glm_vec3_add(camera->transform[3], delta, camera->transform[3]);
}

void camera_mouse_look_3d(struct Camera* camera, vec2 delta)
{
  camera->perspective.yaw -= delta[0] * 0.1f;
  camera->perspective.pitch += delta[1] * 0.1f;

  camera->perspective.pitch = glm_clamp(camera->perspective.pitch, -PITCH_CLAMP, PITCH_CLAMP);

  // Update transform
  yaw_pitch_to_transform_3d(camera);
}

void camera_calc_view_matrix(struct Camera* camera, mat4 view_matrix)
{
  // Keyboard-based movement for perspective cameras
  if (viewport_get_type(camera->viewport) == ViewportType_Perspective)
  {
    vec3 move;
    input_get_camera_movement(move);

    mat3 rot;
    glm_mat4_pick3(camera->transform, rot);

    glm_mat3_mulv(rot, move, move);

    glm_vec3_add(camera->transform[3], move, camera->transform[3]);
  }

  glm_mat4_inv(camera->transform, view_matrix); // Convert transform to view matrix
}

void camera_calc_proj_matrix(struct Camera* camera, mat4 proj_matrix)
{
  vec2 half_viewport_resolution;
  {
    uint32_t width, height;
    viewport_get_resolution(camera->viewport, &width, &height);
    glm_vec2_scale((vec2){ (float)width, (float)height }, 0.5f, half_viewport_resolution);
  }

  const enum ViewportType viewport_type = viewport_get_type(camera->viewport);
  if (viewport_type != ViewportType_Perspective)
  {
    const float world_per_pixel = camera->orthographic.zoom * 0.01f;
    glm_vec2_scale(half_viewport_resolution, world_per_pixel, half_viewport_resolution);

    vec2 from, to;
    glm_vec2_negate_to(half_viewport_resolution, from);
    glm_vec2_copy(half_viewport_resolution, to);

    glm_ortho(from[0], to[0], to[1], from[1], camera->near, camera->far, proj_matrix);
  }
  else
  {
    const float aspect = half_viewport_resolution[0] / half_viewport_resolution[1];
    glm_perspective(camera->perspective.fov, aspect, camera->near, camera->far, proj_matrix);
  }
}