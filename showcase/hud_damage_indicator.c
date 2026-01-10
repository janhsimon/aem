#include "hud_damage_indicator.h"

#include "camera.h"
#include "texture.h"

#include <cglm/affine2d.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#include <glad/gl.h>

#include <string.h>

#define SEGMENT_COUNT 8                     // How many segments are there
#define SEGMENT_DIRECTIONAL_THRESHOLD 0.75f // Trigger segments closer than this threshold
#define SEGMENT_OPACITY 2.0f                // How much a triggered segments lights up
#define SEGMENT_OPACITY_DECAY_SPEED 4.0f    // How fast triggered segments decay

static ImTextureRef segment_texture;

static float segment_opacities[SEGMENT_COUNT];

bool load_hud_damage_indicator()
{
  if (!load_texture("textures/pain.png", (unsigned int*)(&segment_texture._TexID)))
  {
    return false;
  }

  segment_texture._TexData = NULL;

  // Initialize all segment opacities to 0
  memset(segment_opacities, 0, sizeof(segment_opacities));

  return true;
}

static void get_direction_for_segment_index(int index, vec3 dir)
{
  if (index == 0)
  {
    glm_vec3_copy((vec3){ 0.0f, 0.0f, -1.0f }, dir);
  }
  else if (index == 1)
  {
    glm_vec3_copy((vec3){ 1.0f, 0.0f, -1.0f }, dir);
  }
  else if (index == 2)
  {
    glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, dir);
  }
  else if (index == 3)
  {
    glm_vec3_copy((vec3){ 1.0f, 0.0f, 1.0f }, dir);
  }
  else if (index == 4)
  {
    glm_vec3_copy((vec3){ 0.0f, 0.0f, 1.0f }, dir);
  }
  else if (index == 5)
  {
    glm_vec3_copy((vec3){ -1.0f, 0.0f, 1.0f }, dir);
  }
  else if (index == 6)
  {
    glm_vec3_copy((vec3){ -1.0f, 0.0f, 0.0f }, dir);
  }
  else if (index == 7)
  {
    glm_vec3_copy((vec3){ -1.0f, 0.0f, -1.0f }, dir);
  }

  glm_normalize(dir);
}

void hud_damage_indicate(vec3 damage_direction)
{
  glm_normalize(damage_direction);

  // Transform the direction with the inverse camera rotation to get the delta
  mat3 inv_cam_rot;
  cam_calc_rotation(inv_cam_rot, CameraRotationMode_WithoutRecoil);
  glm_mat3_inv(inv_cam_rot, inv_cam_rot);
  glm_mat3_mulv(inv_cam_rot, damage_direction, damage_direction);

  for (int i = 0; i < SEGMENT_COUNT; ++i)
  {
    vec3 segment_direction;
    get_direction_for_segment_index(i, segment_direction);
    if (glm_vec3_dot(damage_direction, segment_direction) > SEGMENT_DIRECTIONAL_THRESHOLD)
    {
      segment_opacities[i] = SEGMENT_OPACITY;
    }
  }
}

void update_hud_damage_indicator(struct ImDrawList* draw_list,
                                 uint32_t half_screen_width,
                                 uint32_t half_screen_height,
                                 float delta_time)
{
  // Decay the segment opacities
  for (int i = 0; i < SEGMENT_COUNT; ++i)
  {
    segment_opacities[i] -= delta_time * SEGMENT_OPACITY_DECAY_SPEED;
  }

  // Build a unit segment for the current screen size
  vec3 segment_vertices[4];
  {
    const float min_screen_size = (float)(glm_min(half_screen_width, half_screen_height) * 2);
    const float aspect_ratio = 1172.0f / 369.0f;
    const float half_h = min_screen_size / 30.0f;
    const float half_w = half_h * aspect_ratio;

    const float dist = min_screen_size / 4.0f;

    segment_vertices[0][0] = -half_w;
    segment_vertices[0][1] = -half_h - dist;
    segment_vertices[0][2] = 1.0f;

    segment_vertices[1][0] = half_w;
    segment_vertices[1][1] = -half_h - dist;
    segment_vertices[1][2] = 1.0f;

    segment_vertices[2][0] = half_w;
    segment_vertices[2][1] = half_h - dist;
    segment_vertices[2][2] = 1.0f;

    segment_vertices[3][0] = -half_w;
    segment_vertices[3][1] = half_h - dist;
    segment_vertices[3][2] = 1.0f;
  }

  // Transform and draw the individual segments
  for (int i = 0; i < SEGMENT_COUNT; ++i)
  {
    mat3 transform;
    glm_translate2d_make(transform, (vec2){ half_screen_width, half_screen_height });
    glm_rotate2d(transform, i * GLM_PI_4f);

    vec3 v[4];
    glm_mat3_mulv(transform, segment_vertices[0], v[0]);
    glm_mat3_mulv(transform, segment_vertices[1], v[1]);
    glm_mat3_mulv(transform, segment_vertices[2], v[2]);
    glm_mat3_mulv(transform, segment_vertices[3], v[3]);

    const ImU32 color = igGetColorU32_Vec4((ImVec4){ 1.0f, 1.0f, 1.0f, segment_opacities[i] });

    ImDrawList_AddImageQuad(draw_list, segment_texture, (ImVec2){ v[0][0], v[0][1] }, (ImVec2){ v[1][0], v[1][1] },
                            (ImVec2){ v[2][0], v[2][1] }, (ImVec2){ v[3][0], v[3][1] }, (ImVec2){ 0.0f, 0.0f },
                            (ImVec2){ 1.0f, 0.0f }, (ImVec2){ 1.0f, 1.0f }, (ImVec2){ 0.0f, 1.0f }, color);
  }
}

void free_hud_damage_indicator()
{
  free_texture(segment_texture._TexID);
}