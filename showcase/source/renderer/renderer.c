#include "renderer.h"

#include "camera.h"
#include "debug_manager.h"
#include "enemy/enemy.h"
#include "forward_pass/depth_pipeline.h"
#include "forward_pass/forward_framebuffer.h"
#include "forward_pass/main_pipeline.h"
#include "forward_pass/particle_pipeline.h"
#include "forward_pass/particle_renderer.h"
#include "forward_pass/tracer_pipeline.h"
#include "forward_pass/tracer_renderer.h"
#include "map.h"
#include "model_renderer.h"
#include "particle_manager.h"
#include "player/player.h"
#include "player/view_model.h"
#include "post_pass/debug_pipeline.h"
#include "post_pass/debug_renderer.h"
#include "post_pass/tonemap_pipeline.h"
#include "preferences.h"
#include "shadow_pass/shadow_framebuffer.h"
#include "shadow_pass/shadow_pipeline.h"
#include "ssao_pass/ssao_blur_pipeline.h"
#include "ssao_pass/ssao_framebuffer.h"
#include "ssao_pass/ssao_pipeline.h"
#include "tracer_manager.h"

#include <cglm/mat4.h>
#include <cglm/vec2.h>

#include <glad/gl.h>

static struct Preferences* preferences;
static uint32_t screen_width, screen_height;
static float screen_aspect;
static float camera_near, camera_far;
static mat4 view_matrix, proj_matrix;

bool load_renderer(struct Preferences* preferences_, uint32_t screen_width, uint32_t screen_height)
{
  preferences = preferences_;

  load_model_renderer();

  // Shadow pass
  {
    if (!load_shadow_framebuffer())
    {
      return false;
    }

    if (!load_shadow_pipeline())
    {
      return false;
    }
  }

  // SSAO pass
  {
    if (!load_ssao_framebuffer(screen_width / 2, screen_height / 2))
    {
      return false;
    }

    if (!load_ssao_pipeline())
    {
      return false;
    }

    if (!load_ssao_blur_pipeline())
    {
      return false;
    }
  }

  // Forward pass
  {
    if (!load_forward_framebuffer(screen_width, screen_height))
    {
      return false;
    }

    if (!load_depth_pipeline())
    {
      return false;
    }

    if (!load_main_pipeline())
    {
      return false;
    }

    if (!load_particle_pipeline())
    {
      return false;
    }

    if (!load_tracer_pipeline())
    {
      return false;
    }

    if (!load_particle_renderer())
    {
      return false;
    }

    if (!load_tracer_renderer())
    {
      return false;
    }
  }

  // Post pass
  {
    if (!load_tonemap_pipeline())
    {
      return false;
    }

    if (!load_debug_pipeline())
    {
      return false;
    }

    load_debug_renderer();
  }

  return true;
}

void free_renderer()
{
  free_model_renderer();

  // Shadow pass
  free_shadow_framebuffer();
  free_shadow_pipeline();

  // SSAO pass
  free_ssao_framebuffer();
  free_ssao_pipeline();
  free_ssao_blur_pipeline();

  // Forward pass
  free_forward_framebuffer();
  free_depth_pipeline();
  free_main_pipeline();
  free_particle_pipeline();
  free_tracer_pipeline();
  free_particle_renderer();
  free_tracer_renderer();

  // Post pass
  free_tonemap_pipeline();
  free_debug_pipeline();
  free_debug_renderer();
}

static void render_shadow_pass()
{
  start_model_rendering();

  shadow_framebuffer_start_rendering();
  shadow_pipeline_start_rendering();
  glClear(GL_DEPTH_BUFFER_BIT);

  shadow_pipeline_calc_light_viewproj(preferences->light_dir, screen_aspect, preferences->camera_fov, camera_near,
                                      camera_far);

  // Map
  {
    shadow_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

    const uint32_t part_count = get_map_part_count();
    for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
    {
      render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly);
    }
  }

  // Enemy
  {
    mat4 enemy_world_matrix;
    get_enemy_world_matrix(enemy_world_matrix);
    shadow_pipeline_use_world_matrix(enemy_world_matrix);

    prepare_enemy_rendering();
    render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly);
  }
}

static void render_forward_pass_early()
{
  start_model_rendering();

  forward_framebuffer_start_rendering(screen_width, screen_height,
                                      ForwardFramebufferAttachment_ViewspaceNormalsTexture);

  depth_pipeline_start_rendering();

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  depth_pipeline_use_view_matrix(view_matrix);
  depth_pipeline_use_proj_matrix(proj_matrix);

  // Map
  {
    depth_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

    const uint32_t part_count = get_map_part_count();
    for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
    {
      render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly);
    }
  }

  // Enemy
  {
    mat4 enemy_world_matrix;
    get_enemy_world_matrix(enemy_world_matrix);
    depth_pipeline_use_world_matrix(enemy_world_matrix);

    prepare_enemy_rendering();
    render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly);
  }
}

static void render_ssao_pass()
{
  glDisable(GL_DEPTH_TEST);

  // SSAO generation
  {
    ssao_framebuffer_start_rendering(screen_width / 2, screen_height / 2, 0);
    ssao_pipeline_start_rendering();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Bind view-space normals texture
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, forward_framebuffer_get_view_space_normals_texture());
    }

    // Bind depth texture
    {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, forward_framebuffer_get_depth_texture());
    }

    ssao_pipeline_use_proj_matrix(proj_matrix);
    ssao_pipeline_use_parameters(preferences->ssao_radius, preferences->ssao_bias, preferences->ssao_strength);

    // Screen size
    {
      vec2 screen_size = { screen_width / 2, screen_height / 2 };
      ssao_pipeline_use_screen_size(screen_size);
    }

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  if (!preferences->ssao_blur)
  {
    return;
  }

  // SSAO horizontal blur
  {
    ssao_framebuffer_start_rendering(screen_width / 2, screen_height / 2, 1);
    ssao_blur_pipeline_start_rendering();

    // Bind SSAO texture
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, ssao_framebuffer_get_texture(0));
    }

    // Bind depth texture
    {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, forward_framebuffer_get_depth_texture());
    }

    // Texel size
    {
      vec2 texel_size = { 1.0f / (float)(screen_width / 2), 1.0f / (float)(screen_height / 2) };
      ssao_blur_pipeline_use_texel_size(texel_size);
    }

    ssao_blur_pipeline_use_full_resolution((vec2){ screen_width, screen_height });
    ssao_blur_pipeline_use_parameters(preferences->ssao_blur_depth_sigma, preferences->ssao_blur_radius);

    ssao_blur_pipeline_use_axis((vec2){ 1.0f, 0.0f }); // Horizontal

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // SSAO vertical blur
  {
    ssao_framebuffer_start_rendering(screen_width / 2, screen_height / 2, 0);
    ssao_blur_pipeline_start_rendering();

    // Bind SSAO texture
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, ssao_framebuffer_get_texture(1));
    }

    ssao_blur_pipeline_use_axis((vec2){ 0.0f, 1.0f }); // Vertical

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  glEnable(GL_DEPTH_TEST);
}

static void render_forward_pass_late()
{
  start_model_rendering();

  forward_framebuffer_start_rendering(screen_width, screen_height, ForwardFramebufferAttachment_HDRTexture);

  main_pipeline_start_rendering();
  glClearColor(preferences->camera_background_color[0], preferences->camera_background_color[1],
               preferences->camera_background_color[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Depth already exists, don't overwrite, use early-Z optimization
  glDepthMask(GL_FALSE); // Don't write depth values
  glDepthFunc(GL_EQUAL); // Early-Z

  // Bind shadow map
  {
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, shadow_framebuffer_get_shadow_texture());
  }

  // Bind SSAO texture
  {
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, ssao_framebuffer_get_texture(0));
  }

  main_pipeline_use_ambient_color(preferences->ambient_color, preferences->ambient_intensity);
  main_pipeline_use_view_matrix(view_matrix);
  main_pipeline_use_proj_matrix(proj_matrix);

  // Screen size
  {
    vec2 screen_size = { screen_width, screen_height };
    main_pipeline_use_screen_size(screen_size);
  }

  // Set the latest camera position
  {
    vec3 camera_position;
    cam_get_position(camera_position);
    main_pipeline_use_camera(camera_position);
  }

  {
    mat4 light_viewproj;
    shadow_pipeline_get_light_viewproj(light_viewproj);
    main_pipeline_use_light(preferences->light_dir, preferences->light_color, preferences->light_intensity,
                            light_viewproj);
  }

  // Main pipeline (opaque)
  {
    main_pipeline_use_render_mode(MainPipelineRenderMode_Opaque);
    main_pipeline_use_ssao(true);

    // Map
    {
      main_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

      const uint32_t part_count = get_map_part_count();
      for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
      {
        render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly);
      }
    }

    // Enemy
    {
      mat4 enemy_world_matrix;
      get_enemy_world_matrix(enemy_world_matrix);
      main_pipeline_use_world_matrix(enemy_world_matrix);

      prepare_enemy_rendering();
      render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly);
    }
  }

  main_pipeline_use_ssao(false);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Main pipeline (transparent)
  {
    main_pipeline_use_render_mode(MainPipelineRenderMode_Transparent);

    // Map
    {
      main_pipeline_use_world_matrix(GLM_MAT4_IDENTITY);

      const uint32_t part_count = get_map_part_count();
      for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
      {
        render_model(get_map_part(map_part_index), ModelRenderMode_TransparentMeshesOnly);
      }
    }
  }

  // Tracer pipeline
  {
    start_tracer_rendering();
    tracer_pipeline_start_rendering();

    tracer_pipeline_use_viewproj_matrix(view_matrix, proj_matrix);
    tracer_pipeline_use_color(preferences->tracer_color);
    tracer_pipeline_use_thickness(preferences->tracer_thickness);

    render_tracer_manager();
  }

  // Particle pipeline
  {
    start_particle_rendering();
    particle_pipeline_start_rendering();

    particle_pipeline_use_viewproj_matrix(view_matrix, proj_matrix);

    render_particle_manager();
  }

  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);

  // Main pipeline (view model)
  if (get_player_health() > 0.0f)
  {
    start_model_rendering();
    main_pipeline_start_rendering();

    glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

    {
      mat4 view_model_world_matrix;
      view_model_get_world_matrix(preferences, view_model_world_matrix);
      main_pipeline_use_world_matrix(view_model_world_matrix);
    }

    {
      mat4 view_model_proj_matrix;
      calc_proj_matrix(screen_aspect, preferences->view_model_fov, camera_near, camera_far, view_model_proj_matrix);
      main_pipeline_use_proj_matrix(view_model_proj_matrix);
    }

    main_pipeline_use_render_mode(MainPipelineRenderMode_Opaque);

    prepare_view_model_rendering();
    render_model(get_view_model_render_info(), ModelRenderMode_OpaqueMeshesOnly);
  }
}

static void render_post_pass()
{
  glDisable(GL_DEPTH_TEST);

  // Tonemap pipeline
  {
    // Render full-screen quad with forward HDR texture to screen
    glViewport(0, 0, screen_width, screen_height);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    tonemap_pipeline_start_rendering();

    // Bind HDR texture
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, forward_framebuffer_get_hdr_texture());
    }

    // Calculate saturation for effect when the player dies
    {
      const float saturation = 1.0f - glm_min(player_get_respawn_cooldown(), 1.0f); // One sec fade
      tonemap_pipeline_use_saturation(saturation);
    }

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Debug pipeline
  if (preferences->debug_render)
  {
    const float aspect = (float)screen_width / (float)screen_height;

    mat4 viewproj_matrix;
    glm_mat4_mul(proj_matrix, view_matrix, viewproj_matrix);

    debug_pipeline_start_rendering();
    debug_pipeline_use_viewproj_matrix(viewproj_matrix);

    // Lines
    render_debug_manager_lines(GLM_YUP);

    // Capsules
    start_debug_rendering_capsules();
    debug_draw_enemy();
  }

  glEnable(GL_DEPTH_TEST);
}

void render_frame(uint32_t screen_width_, uint32_t screen_height_, float camera_near_, float camera_far_)
{
  screen_width = screen_width_;
  screen_height = screen_height_;
  camera_near = camera_near_;
  camera_far = camera_far_;

  screen_aspect = (float)screen_width / (float)screen_height;

  calc_view_matrix(view_matrix);
  calc_proj_matrix(screen_aspect, preferences->camera_fov, camera_near, camera_far, proj_matrix);

  render_shadow_pass();        // Shadow mapping
  render_forward_pass_early(); // Early-Z and view-space normals
  render_ssao_pass();          // SSAO
  render_forward_pass_late();  // HDR shading
  render_post_pass();          // Tonemap
}