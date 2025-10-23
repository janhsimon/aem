#pragma once

#include <stdbool.h>
#include <stdint.h>

struct AEMAnimationMixer;

enum AEMAnimationMixerResult
{
  AEMAnimationMixerResult_Success,
  AEMAnimationMixerResult_OutOfMemory
};

enum AEMAnimationBlendMode
{
  AEMAnimationBlendMode_Linear,
  AEMAnimationBlendMode_Smooth,
  AEMAnimationBlendMode_Smoother
};

struct AEMAnimationChannel
{
  uint32_t animation_index;
  bool is_playing, is_looping;
  float time, playback_speed, weight;
};

enum AEMAnimationMixerResult
aem_load_animation_mixer(uint32_t joint_count, uint32_t channel_count, struct AEMAnimationMixer** mixer);
void aem_free_animation_mixer(struct AEMAnimationMixer* mixer);

bool aem_get_animation_mixer_enabled(const struct AEMAnimationMixer* mixer);
void aem_set_animation_mixer_enabled(struct AEMAnimationMixer* mixer, bool enabled);

float aem_get_animation_mixer_blend_speed(const struct AEMAnimationMixer* mixer);
void aem_set_animation_mixer_blend_speed(struct AEMAnimationMixer* mixer, float blend_speed);

enum AEMAnimationBlendMode aem_get_animation_mixer_blend_mode(const struct AEMAnimationMixer* mixer);
void aem_set_animation_mixer_blend_mode(struct AEMAnimationMixer* mixer, enum AEMAnimationBlendMode blend_mode);

// For manual channel access and modification
struct AEMAnimationChannel*
aem_get_animation_mixer_channel(const struct AEMAnimationMixer* mixer, uint32_t channel_index);

void aem_get_animation_mixer_joint_transform(const struct AEMModel* model,
                                             const struct AEMAnimationMixer* mixer,
                                             uint32_t joint_index,
                                             float transform[16]);

// Convenience functions
void aem_blend_to_animation_mixer_channel(struct AEMAnimationMixer* mixer, uint32_t channel_index);

void aem_update_animation(const struct AEMModel* model,
                          struct AEMAnimationMixer* mixer,
                          float delta_time,
                          float* joint_transforms);
