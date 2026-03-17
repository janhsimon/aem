#include "renderer.h"

#include "bloom_pass/bloom_downsample_pipeline.h"
#include "bloom_pass/bloom_framebuffer.h"
#include "bloom_pass/bloom_prefilter_pipeline.h"
#include "bloom_pass/bloom_upsample_pipeline.h"
#include "camera.h"
#include "debug_manager.h"
#include "debug_texture_pass/debug_texture_framebuffer.h"
#include "debug_texture_pass/frustum_pipeline.h"
#include "debug_texture_pass/frustum_renderer.h"
#include "debug_texture_pass/shadow_composite_pipeline.h"
#include "directional_light.h"
#include "enemy/enemy.h"
#include "forward_pass/depth_pipeline.h"
#include "forward_pass/forward_framebuffer.h"
#include "forward_pass/particle_pipeline.h"
#include "forward_pass/particle_renderer.h"
#include "forward_pass/tracer_pipeline.h"
#include "forward_pass/tracer_renderer.h"
#include "forward_pass/world_pipeline.h"
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
static mat4 view_matrix, proj_matrix;

bool load_renderer(struct Preferences* preferences_, uint32_t screen_width_, uint32_t screen_height_)
{
  preferences = preferences_;
  screen_width = screen_width_;
  screen_height = screen_height_;

  load_model_renderer();

  // Shadow pass
  {
    if (!load_shadow_framebuffer(preferences))
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
    if (!load_ssao_framebuffer(screen_width, screen_height))
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

    if (!load_world_pipeline())
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

  // Bloom pass
  {
    if (!load_bloom_framebuffer(screen_width, screen_height))
    {
      return false;
    }

    if (!load_bloom_prefilter_pipeline())
    {
      return false;
    }

    if (!load_bloom_downsample_pipeline())
    {
      return false;
    }

    if (!load_bloom_upsample_pipeline())
    {
      return false;
    }
  }

  // Debug texture pass
  {
    if (!load_debug_texture_framebuffer())
    {
      return false;
    }

    if (!load_frustum_pipeline())
    {
      return false;
    }

    if (!load_shadow_composite_pipeline())
    {
      return false;
    }

    if (!load_frustum_renderer())
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
  free_world_pipeline();
  free_particle_pipeline();
  free_tracer_pipeline();
  free_particle_renderer();
  free_tracer_renderer();

  // Bloom pass
  free_bloom_framebuffer();
  free_bloom_prefilter_pipeline();
  free_bloom_downsample_pipeline();
  free_bloom_upsample_pipeline();

  // Debug texture pass
  free_debug_texture_framebuffer();
  free_frustum_pipeline();
  free_shadow_composite_pipeline();
  free_frustum_renderer();

  // Post pass
  free_tonemap_pipeline();
  free_debug_pipeline();
  free_debug_renderer();
}

static void render_shadow_pass()
{
  start_model_rendering();

  directional_light_calc_viewproj(preferences, preferences->camera_near, preferences->camera_far);

  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    shadow_framebuffer_start_rendering(preferences, cascade_index);
    glClear(GL_DEPTH_BUFFER_BIT);

    mat4 light_view_matrix, light_proj_matrix;
    directional_light_get_view_matrix(cascade_index, light_view_matrix);
    directional_light_get_proj_matrix(cascade_index, light_proj_matrix);

    // Map (static)
    {
      shadow_pipeline_start_rendering(ShadowPipelineType_Static);
      shadow_pipeline_use_matrices(ShadowPipelineType_Static, GLM_MAT4_IDENTITY, light_view_matrix, light_proj_matrix);

      const uint32_t part_count = get_map_part_count();
      for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
      {
        render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly, false);
      }
    }

    // Enemy (skinned)
    {
      mat4 enemy_world_matrix;
      get_enemy_world_matrix(enemy_world_matrix);

      shadow_pipeline_start_rendering(ShadowPipelineType_Skinned);
      shadow_pipeline_use_matrices(ShadowPipelineType_Skinned, enemy_world_matrix, light_view_matrix,
                                   light_proj_matrix);

      prepare_enemy_rendering();
      render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly, false);
    }
  }
}

static void render_forward_pass_early()
{
  start_model_rendering();

  forward_framebuffer_start_rendering(ForwardFramebufferAttachment_ViewspaceNormalsTexture);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Map (static)
  {
    depth_pipeline_start_rendering(DepthPipelineType_Static);
    depth_pipeline_use_matrices(DepthPipelineType_Static, GLM_MAT4_IDENTITY, view_matrix, proj_matrix);

    const uint32_t part_count = get_map_part_count();
    for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
    {
      render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly, false);
    }
  }

  // Enemy (skinned)
  {
    mat4 enemy_world_matrix;
    get_enemy_world_matrix(enemy_world_matrix);

    depth_pipeline_start_rendering(DepthPipelineType_Skinned);
    depth_pipeline_use_matrices(DepthPipelineType_Skinned, enemy_world_matrix, view_matrix, proj_matrix);

    prepare_enemy_rendering();
    render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly, false);
  }
}

static void render_ssao_pass()
{
  glDisable(GL_DEPTH_TEST);

  // SSAO generation
  {
    ssao_framebuffer_start_rendering(0);
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

  if (preferences->ssao_blur_enable)
  {
    // SSAO horizontal blur
    {
      ssao_framebuffer_start_rendering(1);
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
      ssao_framebuffer_start_rendering(0);
      ssao_blur_pipeline_start_rendering();

      // Bind SSAO texture
      {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssao_framebuffer_get_texture(1));
      }

      ssao_blur_pipeline_use_axis((vec2){ 0.0f, 1.0f }); // Vertical

      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }

  glEnable(GL_DEPTH_TEST);
}

static void render_forward_pass_late()
{
  start_model_rendering();

  forward_framebuffer_start_rendering(ForwardFramebufferAttachment_HDRTexture);

  glClearColor(preferences->camera_background_color[0], preferences->camera_background_color[1],
               preferences->camera_background_color[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Depth already exists, don't overwrite, use early-Z optimization
  glDepthMask(GL_FALSE); // Don't write depth values
  glDepthFunc(GL_EQUAL); // Early-Z

  // Bind shadow map
  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    glActiveTexture(GL_TEXTURE4 + cascade_index);
    glBindTexture(GL_TEXTURE_2D, shadow_framebuffer_get_shadow_cascade(cascade_index));
  }

  // Bind SSAO texture
  if (preferences->ssao_enable)
  {
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, ssao_framebuffer_get_texture(0));
  }

  // Calculate uniform values to use this frame
  vec2 screen_size;
  vec3 camera_position, camera_direction;
  mat4 light_viewproj[4];
  float cascade_splits[3];
  {
    glm_vec2_copy((vec2){ screen_width, screen_height }, screen_size);

    camera_get_position(camera_position);
    camera_get_forward_with_recoil(camera_direction);

    for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
    {
      directional_light_get_viewproj_matrix(cascade_index, light_viewproj[cascade_index]);
    }

    for (uint32_t split_index = 0; split_index < 3; ++split_index)
    {
      cascade_splits[split_index] =
        preferences->camera_near +
        (preferences->camera_far - preferences->camera_near) * preferences->shadow_mapping_cascade_splits[split_index];
    }
  }

  // World pipeline (opaque)
  {
    // Map (static)
    {
      world_pipeline_start_rendering(WorldPipelineType_Static);
      world_pipeline_use_ambient_color(WorldPipelineType_Static, preferences->ambient_color,
                                       preferences->ambient_intensity);
      world_pipeline_use_screen_size(WorldPipelineType_Static, screen_size);
      world_pipeline_use_camera(WorldPipelineType_Static, camera_position, camera_direction);
      world_pipeline_use_light(WorldPipelineType_Static, preferences->light_dir, preferences->light_color,
                               preferences->light_intensity);
      world_pipeline_enable_shadow_mapping(WorldPipelineType_Static, preferences->shadow_mapping_enable,
                                           preferences->shadow_mapping_visualize_cascades);
      world_pipeline_use_render_mode(WorldPipelineType_Static, WorldPipelineRenderMode_Opaque);
      world_pipeline_enable_ssao(WorldPipelineType_Static, preferences->ssao_enable);

      if (preferences->shadow_mapping_enable)
      {
        world_pipeline_use_shadow_cascades(WorldPipelineType_Static, light_viewproj, cascade_splits);
        world_pipeline_use_shadow_parameters(WorldPipelineType_Static, preferences->shadow_mapping_bias,
                                             preferences->shadow_mapping_pcf_radius,
                                             preferences->shadow_mapping_pcf_kernel_size);
      }

      world_pipeline_use_matrices(WorldPipelineType_Static, GLM_MAT4_IDENTITY, view_matrix, proj_matrix);

      const uint32_t part_count = get_map_part_count();
      for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
      {
        render_model(get_map_part(map_part_index), ModelRenderMode_OpaqueMeshesOnly, true);
      }
    }

    // Enemy (skinned)
    {
      world_pipeline_start_rendering(WorldPipelineType_Skinned);
      world_pipeline_use_ambient_color(WorldPipelineType_Skinned, preferences->ambient_color,
                                       preferences->ambient_intensity);
      world_pipeline_use_screen_size(WorldPipelineType_Skinned, screen_size);
      world_pipeline_use_camera(WorldPipelineType_Skinned, camera_position, camera_direction);
      world_pipeline_use_light(WorldPipelineType_Skinned, preferences->light_dir, preferences->light_color,
                               preferences->light_intensity);
      world_pipeline_enable_shadow_mapping(WorldPipelineType_Skinned, preferences->shadow_mapping_enable,
                                           preferences->shadow_mapping_visualize_cascades);
      world_pipeline_use_render_mode(WorldPipelineType_Skinned, WorldPipelineRenderMode_Opaque);
      world_pipeline_enable_ssao(WorldPipelineType_Skinned, preferences->ssao_enable);

      if (preferences->shadow_mapping_enable)
      {
        world_pipeline_use_shadow_cascades(WorldPipelineType_Skinned, light_viewproj, cascade_splits);
        world_pipeline_use_shadow_parameters(WorldPipelineType_Skinned, preferences->shadow_mapping_bias,
                                             preferences->shadow_mapping_pcf_radius,
                                             preferences->shadow_mapping_pcf_kernel_size);
      }

      mat4 enemy_world_matrix;
      get_enemy_world_matrix(enemy_world_matrix);
      world_pipeline_use_matrices(WorldPipelineType_Skinned, enemy_world_matrix, view_matrix, proj_matrix);

      prepare_enemy_rendering();
      render_model(get_enemy_render_info(), ModelRenderMode_OpaqueMeshesOnly, true);
    }
  }

  world_pipeline_start_rendering(WorldPipelineType_Static);
  world_pipeline_enable_ssao(WorldPipelineType_Static, false);

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // World pipeline (transparent)
  {
    world_pipeline_use_render_mode(WorldPipelineType_Static, WorldPipelineRenderMode_Transparent);

    // Map
    {
      world_pipeline_use_matrices(WorldPipelineType_Static, GLM_MAT4_IDENTITY, view_matrix, proj_matrix);

      const uint32_t part_count = get_map_part_count();
      for (uint32_t map_part_index = 0; map_part_index < part_count; ++map_part_index)
      {
        render_model(get_map_part(map_part_index), ModelRenderMode_TransparentMeshesOnly, true);
      }
    }
  }

  // Tracer pipeline
  {
    start_tracer_rendering();
    tracer_pipeline_start_rendering();

    tracer_pipeline_use_viewproj_matrix(view_matrix, proj_matrix);
    tracer_pipeline_use_parameters(preferences->tracer_brightness, preferences->tracer_color,
                                   preferences->tracer_thickness);

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

  // World pipeline (view model)
  if (get_player_health() > 0.0f)
  {
    start_model_rendering();
    world_pipeline_start_rendering(WorldPipelineType_Skinned);

    glClear(GL_DEPTH_BUFFER_BIT); // Clear depth so view model never clips into level

    {
      mat4 view_model_world_matrix, view_model_proj_matrix;
      view_model_get_world_matrix(preferences, view_model_world_matrix);
      camera_get_view_model_proj_matrix(view_model_proj_matrix);
      world_pipeline_use_matrices(WorldPipelineType_Skinned, view_model_world_matrix, view_matrix,
                                  view_model_proj_matrix);
    }

    world_pipeline_use_render_mode(WorldPipelineType_Skinned, WorldPipelineRenderMode_Opaque);

    prepare_view_model_rendering();
    render_model(get_view_model_render_info(), ModelRenderMode_OpaqueMeshesOnly, true);
  }
}

static void render_bloom_pass()
{
  glDisable(GL_DEPTH_TEST);

  // Bloom prefilter
  {
    bloom_framebuffer_start_rendering(0, BloomFramebufferPhase_Downsample);
    bloom_prefilter_pipeline_start_rendering();

    // Bind HDR texture
    {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, forward_framebuffer_get_hdr_texture());
    }

    bloom_prefilter_pipeline_use_parameters(preferences->bloom_threshold, preferences->bloom_soft_knee);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Bloom downsample
  {
    const int downsample_texture_count = bloom_framebuffer_get_texture_count(BloomFramebufferPhase_Downsample);
    for (int source_texture_index = 0; source_texture_index < downsample_texture_count - 1; ++source_texture_index)
    {
      const int destination_texture_index = source_texture_index + 1;

      // Render into the destination texture
      bloom_framebuffer_start_rendering(destination_texture_index, BloomFramebufferPhase_Downsample);
      bloom_downsample_pipeline_start_rendering();

      // Bind source texture
      {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,
                      bloom_framebuffer_get_texture(source_texture_index, BloomFramebufferPhase_Downsample));
      }

      // Source resolution
      {
        uint32_t source_width, source_height;
        bloom_framebuffer_get_texture_resolution(source_texture_index, &source_width, &source_height);
        bloom_downsample_pipeline_use_source_resolution(source_width, source_height);
      }

      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }

  // Bloom upsample
  {
    const int upsample_texture_count = bloom_framebuffer_get_texture_count(BloomFramebufferPhase_Upsample);
    for (int source_texture_index = upsample_texture_count; source_texture_index > 0; --source_texture_index)
    {
      const int destination_texture_index = source_texture_index - 1;

      bloom_framebuffer_start_rendering(destination_texture_index, BloomFramebufferPhase_Upsample);
      bloom_upsample_pipeline_start_rendering();

      // Bind source textures
      {
        // Low
        {
          // Read the last downsample texture for the first iteration, otherwise read the latest upsample texture
          enum BloomFramebufferPhase phase = (source_texture_index == upsample_texture_count) ?
                                               BloomFramebufferPhase_Downsample :
                                               BloomFramebufferPhase_Upsample;

          glActiveTexture(GL_TEXTURE0);
          glBindTexture(GL_TEXTURE_2D, bloom_framebuffer_get_texture(source_texture_index, phase));
        }

        // High
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,
                      bloom_framebuffer_get_texture(destination_texture_index, BloomFramebufferPhase_Downsample));
      }

      // Low resolution
      {
        uint32_t w, h;
        bloom_framebuffer_get_texture_resolution(source_texture_index, &w, &h);
        bloom_upsample_pipeline_use_low_resolution((vec2){ w, h });
      }

      bloom_upsample_pipeline_use_intensity(preferences->bloom_intensity);

      glDrawArrays(GL_TRIANGLES, 0, 3);
    }
  }

  glEnable(GL_DEPTH_TEST);
}

static void render_debug_texture_pass()
{
  for (uint32_t cascade_index = 0; cascade_index < 4; ++cascade_index)
  {
    // Render the camera frustum
    {
      start_frustum_rendering();

      debug_texture_framebuffer_start_rendering(DebugTextureFramebufferAttachment_CameraFrustum, 0);
      frustum_pipeline_start_rendering();
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      mat4 light_viewproj;
      directional_light_get_viewproj_matrix(cascade_index, light_viewproj);
      frustum_pipeline_use_viewproj_matrix(light_viewproj);

      render_frustum(cascade_index, screen_aspect, preferences->camera_fov, preferences->camera_near,
                     preferences->camera_far);
    }

    // Composite
    {
      debug_texture_framebuffer_start_rendering(DebugTextureFramebufferAttachment_ShadowMap, cascade_index);
      shadow_composite_pipeline_start_rendering();

      shadow_composite_pipeline_use_tint(GLM_VEC3_ONE);

      glActiveTexture(GL_TEXTURE0);

      glBindTexture(GL_TEXTURE_2D, shadow_framebuffer_get_shadow_cascade(cascade_index));
      glDrawArrays(GL_TRIANGLES, 0, 3);

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      shadow_composite_pipeline_use_tint(GLM_ZUP);

      glBindTexture(GL_TEXTURE_2D, debug_texture_framebuffer_get_camera_frustum_texture());
      glDrawArrays(GL_TRIANGLES, 0, 3);

      glDisable(GL_BLEND);
    }
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

    // Bind bloom texture
    if (preferences->bloom_enable)
    {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, bloom_framebuffer_get_texture(0, BloomFramebufferPhase_Upsample));
    }

    // Calculate saturation for effect when the player dies
    {
      const float saturation = 1.0f - glm_min(player_get_respawn_cooldown(), 1.0f); // One sec fade
      tonemap_pipeline_use_saturation(saturation);
    }

    tonemap_pipeline_use_bloom(preferences->bloom_enable);

    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Debug pipeline
  if (preferences->debug_render)
  {
    const float aspect = (float)screen_width / (float)screen_height;

    debug_pipeline_start_rendering();

    mat4 viewproj_matrix;
    camera_get_viewproj_matrix(viewproj_matrix);
    debug_pipeline_use_viewproj_matrix(viewproj_matrix);

    // Lines
    render_debug_manager_lines(GLM_YUP);

    // Capsules
    start_debug_rendering_capsules();
    debug_draw_enemy();
  }

  glEnable(GL_DEPTH_TEST);
}

void renderer_on_screen_resize(uint32_t screen_width_, uint32_t screen_height_)
{
  screen_width = screen_width_;
  screen_height = screen_height_;

  // Ignore when the window gets minimized
  if (screen_width == 0 || screen_height == 0)
  {
    return;
  }

  ssao_framebuffer_on_screen_resize(screen_width, screen_height);
  forward_framebuffer_on_screen_resize(screen_width, screen_height);
  bloom_framebuffer_on_screen_resize(screen_width, screen_height);
}

void render_frame()
{
  // Ignore when the window gets minimized
  if (screen_width == 0 || screen_height == 0)
  {
    return;
  }

  screen_aspect = (float)screen_width / (float)screen_height;

  camera_calc_matrices(screen_aspect, preferences->camera_fov, preferences->view_model_fov, preferences->camera_near,
                       preferences->camera_far);
  camera_get_view_matrix(view_matrix);
  camera_get_proj_matrix(proj_matrix);

  if (preferences->shadow_mapping_enable)
  {
    render_shadow_pass(); // Shadow mapping
  }

  render_forward_pass_early(); // Early-Z and view-space normals

  if (preferences->ssao_enable)
  {
    render_ssao_pass(); // SSAO
  }

  render_forward_pass_late(); // HDR shading

  if (preferences->bloom_enable)
  {
    render_bloom_pass(); // Bloom
  }

  if (get_show_shadow_map_window())
  {
    render_debug_texture_pass(); // Prepare textures for debug visualization
  }

  render_post_pass(); // Tonemap
}