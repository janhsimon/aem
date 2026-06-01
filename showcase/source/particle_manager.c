#include "particle_manager.h"

#include "preferences.h"
#include "renderer/forward_pass/particle_renderer.h"

#include <cglm/vec3.h>

#include <stdbool.h>
#include <stdint.h>

#define MAX_PARTICLES 10000

struct ParticleSystem
{
  // Emitter
  uint32_t emitter_particle_count; // How many particles to emit in one go
  vec3 emitter_position;
  float emitter_radius;
  float emitter_direction_spread;
  bool emitter_billboard; // Should particles from this emitter billboard?
  bool emitter_following; // Should spawned particles follow the emitter when it moves?

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
  uint32_t particle_count;                 // How many particles are alive
  uint32_t particle_texture_index;         // 0: Muzzleflash, 1: Smoke etc.
  vec3 particle_positions[MAX_PARTICLES];  // GPU
  vec3 particle_directions[MAX_PARTICLES]; // GPU
  vec3 particle_velocities[MAX_PARTICLES]; // CPU
  float particle_scales[MAX_PARTICLES];    // GPU
  float particle_opacities[MAX_PARTICLES]; // GPU
  float particle_lifetimes[MAX_PARTICLES]; // CPU
} smoke, shrapnel, muzzleflash, blood, bullet_hole;

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
  // Smoke
  {
    smoke.emitter_following = false;
    smoke.emitter_billboard = true;
    smoke.particle_lifetime = -1.0f;
    smoke.particle_texture_index = 1;
    smoke.particle_count = 0;
  }

  // Shrapnel
  {
    shrapnel.emitter_following = false;
    shrapnel.emitter_billboard = true;
    shrapnel.particle_lifetime = -1.0f;
    shrapnel.particle_texture_index = 0;
    shrapnel.particle_count = 0;
  }

  // Muzzleflash
  {
    muzzleflash.emitter_following = true;
    muzzleflash.emitter_billboard = true;
    muzzleflash.particle_lifetime = 0.05f;
    muzzleflash.particle_texture_index = 0;
    muzzleflash.particle_count = 0;
  }

  // Blood
  {
    blood.emitter_following = false;
    blood.emitter_billboard = true;
    blood.particle_lifetime = -1.0f;
    blood.particle_texture_index = 2;
    blood.particle_count = 0;
  }

  // Bullet hole
  {
    bullet_hole.emitter_following = false;
    bullet_hole.emitter_billboard = false;
    bullet_hole.particle_lifetime = -1.0f;
    bullet_hole.particle_texture_index = 3;
    bullet_hole.particle_count = 0;
  }
}

static void fire_particle_system(struct ParticleSystem* system, vec3 position, vec3 dir)
{
  glm_vec3_copy(position, system->emitter_position);

  if (system->particle_count + system->emitter_particle_count >= MAX_PARTICLES)
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
      glm_vec3_copy(dir, system->particle_directions[particle_index]);
    }

    // Velocity
    if (system->emitter_direction_spread > 0.0f)
    {
      vec3 spread;
      point_in_unit_sphere((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, system->emitter_direction_spread, spread);
      glm_vec3_add(dir, spread, system->particle_velocities[particle_index]);
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
  fire_particle_system(&smoke, position, dir);
}

void spawn_shrapnel(vec3 position, vec3 dir)
{
  fire_particle_system(&shrapnel, position, dir);
}

void spawn_muzzleflash(vec3 position)
{
  fire_particle_system(&muzzleflash, position, GLM_VEC3_ZERO);
}

void spawn_blood(vec3 position, vec3 dir)
{
  fire_particle_system(&blood, position, dir);
}

void spawn_bullet_hole(vec3 position, vec3 dir)
{
  fire_particle_system(&bullet_hole, position, dir);
}

void set_muzzleflash_position(vec3 position)
{
  glm_vec3_copy(position, muzzleflash.emitter_position);
}

static void sync_particle_system(struct ParticleSystem* particle_system, struct ParticleSystemPreferences* preferences)
{
  particle_system->emitter_particle_count = preferences->particle_count;
  particle_system->emitter_direction_spread = preferences->direction_spread;
  particle_system->emitter_radius = preferences->radius;

  particle_system->particle_gravity = preferences->gravity;
  particle_system->particle_additive = preferences->additive;
  particle_system->particle_brightness = preferences->brightness;
  glm_vec3_copy(preferences->tint, particle_system->particle_tint);

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
  sync_particle_system(&muzzleflash, &preferences->muzzleflash_particle_system);
  sync_particle_system(&blood, &preferences->blood_particle_system);
  sync_particle_system(&bullet_hole, &preferences->bullet_hole_particle_system);
}

static void update_particle_system(struct ParticleSystem* system, float delta_time)
{
  for (uint32_t particle_index = 0; particle_index < system->particle_count; ++particle_index)
  {
    // Animate particle positions
    // TODO: This is a hack, allow for particle positions while following the emitter
    if (system->emitter_following)
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
  update_particle_system(&muzzleflash, delta_time);
  update_particle_system(&blood, delta_time);
  update_particle_system(&bullet_hole, delta_time);
}

void render_particle_manager()
{
  render_particles(smoke.particle_positions, smoke.particle_directions, smoke.particle_scales, smoke.particle_opacities,
                   smoke.particle_count, smoke.emitter_billboard, smoke.particle_additive, smoke.particle_brightness,
                   smoke.particle_tint, smoke.particle_texture_index);

  render_particles(shrapnel.particle_positions, shrapnel.particle_directions, shrapnel.particle_scales,
                   shrapnel.particle_opacities, shrapnel.particle_count, shrapnel.emitter_billboard,
                   shrapnel.particle_additive, shrapnel.particle_brightness, shrapnel.particle_tint,
                   shrapnel.particle_texture_index);

  render_particles(muzzleflash.particle_positions, muzzleflash.particle_directions, muzzleflash.particle_scales,
                   muzzleflash.particle_opacities, muzzleflash.particle_count, muzzleflash.emitter_billboard,
                   muzzleflash.particle_additive, muzzleflash.particle_brightness, muzzleflash.particle_tint,
                   muzzleflash.particle_texture_index);

  render_particles(blood.particle_positions, blood.particle_directions, blood.particle_scales, blood.particle_opacities,
                   blood.particle_count, blood.emitter_billboard, blood.particle_additive, blood.particle_brightness,
                   blood.particle_tint, blood.particle_texture_index);

  render_particles(bullet_hole.particle_positions, bullet_hole.particle_directions, bullet_hole.particle_scales,
                   bullet_hole.particle_opacities, bullet_hole.particle_count, bullet_hole.emitter_billboard,
                   bullet_hole.particle_additive, bullet_hole.particle_brightness, bullet_hole.particle_tint,
                   bullet_hole.particle_texture_index);
}