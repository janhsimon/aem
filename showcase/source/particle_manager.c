#include "particle_manager.h"

#include "preferences.h"
#include "renderer/forward_pass/particle_renderer.h"

#include <cglm/vec3.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_PARTICLES 10000

struct ParticleSystem
{
  // Emitter
  uint32_t max_particle_count;
  uint32_t emitter_particle_count; // How many particles to emit in one go
  vec3 emitter_position;
  float emitter_radius;
  float emitter_direction_spread;
  bool emitter_billboard; // Should particles from this emitter billboard?
  bool emitter_sticky;    // Should spawned particles follow the emitter when it moves?

  // Particle properties
  float particle_gravity;
  bool particle_additive; // Blend mode: Additive or alpha blending
  float particle_brightness;
  vec3 particle_tint;
  float particle_lifetime; // -1.0: Forever
  float particle_initial_opacity;
  float particle_opacity_spread;
  float particle_opacity_falloff;
  float particle_initial_scale;
  float particle_scale_spread;
  float particle_scale_falloff;

  // Particle data
  uint32_t particle_count;                   // How many particles are alive
  uint32_t particle_texture_index;           // 0: Muzzleflash, 1: Smoke etc.
  vec3 particle_positions[MAX_PARTICLES];    // GPU
  vec4 particle_orientations[MAX_PARTICLES]; // GPU, quaternion XYZW order
  vec3 particle_velocities[MAX_PARTICLES];   // CPU
  float particle_scales[MAX_PARTICLES];      // GPU
  float particle_opacities[MAX_PARTICLES];   // GPU
  float particle_lifetimes[MAX_PARTICLES];   // CPU
} smoke, shrapnel, muzzleflash_player, muzzleflash_enemy_front, muzzleflash_enemy_side, blood, bullet_hole;

static void point_in_unit_sphere(float u1, float u2, float u3, vec3 p)
{
  float phi = u1 * 2.0f * GLM_PI;
  float theta = acosf(2.0f * u2 - 1.0f);

  float r = cbrtf(u3);

  p[0] = r * sinf(theta) * cosf(phi);
  p[1] = r * sinf(theta) * sinf(phi);
  p[2] = r * cosf(theta);
}

void load_particle_manager()
{
  smoke.particle_count = 0;
  shrapnel.particle_count = 0;
  muzzleflash_player.particle_count = 0;
  muzzleflash_enemy_front.particle_count = 0;
  muzzleflash_enemy_side.particle_count = 0;
  blood.particle_count = 0;
  bullet_hole.particle_count = 0;
}

static void fire_particle_system(struct ParticleSystem* system, vec3 position, vec3 emit_dir, vec4 particle_orientation)
{
  glm_vec3_copy(position, system->emitter_position);

  if (system->particle_count + system->emitter_particle_count > system->max_particle_count)
  {
    system->particle_count = 0;
  }

  for (uint32_t i = 0; i < system->emitter_particle_count; ++i)
  {
    const uint32_t particle_index = system->particle_count + i;

    system->particle_lifetimes[particle_index] = system->particle_lifetime;

    // Position
    {
      vec3 r = GLM_VEC3_ZERO_INIT;
      if (system->emitter_radius > 0.0f)
      {
        point_in_unit_sphere((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, system->emitter_radius, r);
      }
      glm_vec3_add(position, r, system->particle_positions[particle_index]);
    }

    // Direction
    if (!system->emitter_billboard)
    {
      glm_vec4_copy(particle_orientation, system->particle_orientations[particle_index]);
    }

    // Velocity
    if (system->emitter_direction_spread > 0.0f)
    {
      vec3 spread;
      point_in_unit_sphere((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, system->emitter_direction_spread, spread);
      glm_vec3_add(emit_dir, spread, system->particle_velocities[particle_index]);
    }

    // Opacity
    system->particle_opacities[particle_index] = system->particle_initial_opacity - system->particle_opacity_spread +
                                                 ((rand() % 1000) / 1000.0f) * system->particle_opacity_spread * 2.0f;

    // Scale
    system->particle_scales[particle_index] = system->particle_initial_scale - system->particle_scale_spread +
                                              ((rand() % 1000) / 1000.0f) * system->particle_scale_spread * 2.0f;
  }

  system->particle_count += system->emitter_particle_count;
}

void spawn_smoke(vec3 position, vec3 dir)
{
  fire_particle_system(&smoke, position, dir, GLM_VEC4_ZERO);
}

void spawn_shrapnel(vec3 position, vec3 dir)
{
  fire_particle_system(&shrapnel, position, dir, GLM_VEC4_ZERO);
}

void spawn_player_muzzleflash(vec3 position)
{
  fire_particle_system(&muzzleflash_player, position, GLM_VEC3_ZERO, GLM_VEC4_ZERO);
}

void spawn_enemy_muzzleflash(vec3 front_position,
                             versor front_orientation,
                             vec3 side_position,
                             versor side_orientation1,
                             versor side_orientation2)
{
  fire_particle_system(&muzzleflash_enemy_front, front_position, GLM_VEC3_ZERO, front_orientation);
  fire_particle_system(&muzzleflash_enemy_side, side_position, GLM_VEC3_ZERO, side_orientation1);
  fire_particle_system(&muzzleflash_enemy_side, side_position, GLM_VEC3_ZERO, side_orientation2);
}

void spawn_blood(vec3 position, vec3 dir)
{
  fire_particle_system(&blood, position, dir, GLM_VEC4_ZERO);
}

static void quat_look_rotation(vec3 direction, vec3 up, versor q)
{
  vec3 f, r, u;

  glm_vec3_normalize_to(direction, f);

  glm_vec3_cross(up, f, r);
  glm_vec3_normalize(r);

  glm_vec3_cross(f, r, u);

  mat3 rot = { { r[0], r[1], r[2] }, { u[0], u[1], u[2] }, { f[0], f[1], f[2] } };

  glm_mat3_quat(rot, q);
  glm_quat_normalize(q);
}

void spawn_bullet_hole(vec3 position, vec3 dir)
{
  glm_normalize(dir);

  vec3 local_up = { 0.0f, 1.0f, 0.0f };
  if (fabs(dir[1] > 0.999f))
  {
    glm_vec3_copy((vec3){ 1.0f, 0.0f, 0.0f }, local_up);
  }

  const float random_roll = ((rand() % 100) / 100.0f) * GLM_PI * 2.0f;
  glm_vec3_rotate(local_up, random_roll, dir);

  versor q;
  quat_look_rotation(dir, local_up, q);
  fire_particle_system(&bullet_hole, position, GLM_VEC3_ZERO, q);
}

void set_player_muzzleflash_position(vec3 position)
{
  glm_vec3_copy(position, muzzleflash_player.emitter_position);
}

static void sync_particle_system(struct ParticleSystem* particle_system, struct ParticleSystemPreferences* preferences)
{
  particle_system->emitter_particle_count = preferences->particle_count;
  particle_system->max_particle_count = preferences->max_particle_count;

  particle_system->emitter_billboard = preferences->billboard;
  particle_system->emitter_sticky = preferences->sticky;
  particle_system->particle_additive = preferences->additive;

  particle_system->particle_texture_index = preferences->texture_index;
  particle_system->particle_brightness = preferences->brightness;
  glm_vec3_copy(preferences->tint, particle_system->particle_tint);

  particle_system->particle_lifetime = preferences->lifetime;

  particle_system->emitter_direction_spread = preferences->direction_spread;
  particle_system->emitter_radius = preferences->radius;

  particle_system->particle_gravity = preferences->gravity;

  particle_system->particle_initial_opacity = preferences->opacity;
  particle_system->particle_opacity_spread = preferences->opacity_spread;
  particle_system->particle_opacity_falloff = preferences->opacity_falloff;

  particle_system->particle_initial_scale = preferences->scale;
  particle_system->particle_scale_spread = preferences->scale_spread;
  particle_system->particle_scale_falloff = preferences->scale_falloff;
}

void sync_particle_manager(struct Preferences* preferences)
{
  sync_particle_system(&smoke, &preferences->smoke_particle_system);
  sync_particle_system(&shrapnel, &preferences->shrapnel_particle_system);
  sync_particle_system(&muzzleflash_player, &preferences->muzzleflash_player_particle_system);
  sync_particle_system(&muzzleflash_enemy_front, &preferences->muzzleflash_enemy_front_particle_system);
  sync_particle_system(&muzzleflash_enemy_side, &preferences->muzzleflash_enemy_side_particle_system);
  sync_particle_system(&blood, &preferences->blood_particle_system);
  sync_particle_system(&bullet_hole, &preferences->bullet_hole_particle_system);
}

static void update_particle_system(struct ParticleSystem* system, float delta_time)
{
  for (uint32_t particle_index = 0; particle_index < system->particle_count; ++particle_index)
  {
    // Animate particle positions
    // TODO: This is a hack, allow for particle positions while following the emitter
    if (system->emitter_sticky)
    {
      glm_vec3_copy(system->emitter_position, system->particle_positions[particle_index]);
    }
    else
    {
      system->particle_velocities[particle_index][1] -= system->particle_gravity * delta_time;

      vec3 scaled_velocity;
      glm_vec3_scale(system->particle_velocities[particle_index], delta_time, scaled_velocity);

      glm_vec3_add(system->particle_positions[particle_index], scaled_velocity,
                   system->particle_positions[particle_index]);
    }

    // Fade out particles over time
    // TODO: Delta time this
    system->particle_scales[particle_index] *= 1.0f - system->particle_scale_falloff;
    system->particle_opacities[particle_index] *= 1.0f - system->particle_opacity_falloff;

    // Handle overall particle lifetime
    // TODO: Do proper lifetime management here instead of this hack
    if (system->particle_lifetimes[particle_index] > 0.0f)
    {
      system->particle_lifetimes[particle_index] -= delta_time;

      if (system->particle_lifetimes[particle_index] <= 0.0f)
      {
        system->particle_scales[particle_index] = 0.0f;
      }
    }
  }
}

void update_particle_manager(float delta_time)
{
  update_particle_system(&smoke, delta_time);
  update_particle_system(&shrapnel, delta_time);
  update_particle_system(&muzzleflash_player, delta_time);
  update_particle_system(&muzzleflash_enemy_front, delta_time);
  update_particle_system(&muzzleflash_enemy_side, delta_time);
  update_particle_system(&blood, delta_time);
  update_particle_system(&bullet_hole, delta_time);
}

void render_particle_manager()
{
  render_particles(smoke.particle_positions, smoke.particle_orientations, smoke.particle_scales,
                   smoke.particle_opacities, smoke.particle_count, smoke.emitter_billboard, smoke.particle_additive,
                   smoke.particle_brightness, smoke.particle_tint, smoke.particle_texture_index);

  render_particles(shrapnel.particle_positions, shrapnel.particle_orientations, shrapnel.particle_scales,
                   shrapnel.particle_opacities, shrapnel.particle_count, shrapnel.emitter_billboard,
                   shrapnel.particle_additive, shrapnel.particle_brightness, shrapnel.particle_tint,
                   shrapnel.particle_texture_index);

  render_particles(muzzleflash_player.particle_positions, muzzleflash_player.particle_orientations,
                   muzzleflash_player.particle_scales, muzzleflash_player.particle_opacities,
                   muzzleflash_player.particle_count, muzzleflash_player.emitter_billboard,
                   muzzleflash_player.particle_additive, muzzleflash_player.particle_brightness,
                   muzzleflash_player.particle_tint, muzzleflash_player.particle_texture_index);

  render_particles(muzzleflash_enemy_front.particle_positions, muzzleflash_enemy_front.particle_orientations,
                   muzzleflash_enemy_front.particle_scales, muzzleflash_enemy_front.particle_opacities,
                   muzzleflash_enemy_front.particle_count, muzzleflash_enemy_front.emitter_billboard,
                   muzzleflash_enemy_front.particle_additive, muzzleflash_enemy_front.particle_brightness,
                   muzzleflash_enemy_front.particle_tint, muzzleflash_enemy_front.particle_texture_index);

  render_particles(muzzleflash_enemy_side.particle_positions, muzzleflash_enemy_side.particle_orientations,
                   muzzleflash_enemy_side.particle_scales, muzzleflash_enemy_side.particle_opacities,
                   muzzleflash_enemy_side.particle_count, muzzleflash_enemy_side.emitter_billboard,
                   muzzleflash_enemy_side.particle_additive, muzzleflash_enemy_side.particle_brightness,
                   muzzleflash_enemy_side.particle_tint, muzzleflash_enemy_side.particle_texture_index);

  render_particles(blood.particle_positions, blood.particle_orientations, blood.particle_scales,
                   blood.particle_opacities, blood.particle_count, blood.emitter_billboard, blood.particle_additive,
                   blood.particle_brightness, blood.particle_tint, blood.particle_texture_index);

  render_particles(bullet_hole.particle_positions, bullet_hole.particle_orientations, bullet_hole.particle_scales,
                   bullet_hole.particle_opacities, bullet_hole.particle_count, bullet_hole.emitter_billboard,
                   bullet_hole.particle_additive, bullet_hole.particle_brightness, bullet_hole.particle_tint,
                   bullet_hole.particle_texture_index);
}