#include "animation_mixer.h"

#include "model.h"

#include <aem/animation_mixer.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

#define PLAYBACK_BUTTON_WIDTH 50.0f

char** animation_names = NULL;
uint32_t animation_name_count = 0;

static void free_animation_names()
{
  if (!animation_names)
  {
    return;
  }

  for (uint32_t animation_name_index = 0; animation_name_index < animation_name_count; ++animation_name_index)
  {
    free(animation_names[animation_name_index]);
  }

  free(animation_names);
  animation_names = NULL;
}

void animation_mixer_on_new_model()
{
  free_animation_names();

  animation_names = get_model_animation_names();
  animation_name_count = get_model_animation_count();
}

void update_animation_mixer(int screen_width, int screen_height)
{
  igSetNextWindowPos((struct ImVec2){ 0.0f, screen_height }, 0, (struct ImVec2){ 0.0f, 1.0f });
  igSetNextWindowSize((struct ImVec2){ screen_width, 0.0f }, 0);

  igBegin("##AnimationMixer", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

  if (!animation_names)
  {
    set_model_animation_mixer_enabled(false);
    igBeginDisabled(true);
  }

  bool enable = get_model_animation_mixer_enabled();
  if (igCheckbox("Enable Animation Mixer", &enable))
  {
    set_model_animation_mixer_enabled(enable);
  }

  if (!animation_names)
  {
    igEndDisabled();
  }

  igBeginDisabled(!enable);

  // Blend mode
  {
    igSameLine(0.0f, 100.0f);
    igText("Blend Mode:");

    igSameLine(0.0f, -1.0f);
    igSetNextItemWidth(150.0f);

    enum AEMAnimationBlendMode blend_mode = get_model_animation_mixer_blend_mode();
    static const char* blend_mode_names[] = { "Linear", "Smooth", "Smoother" };
    if (igBeginCombo("##AnimationMixerBlendMode", blend_mode_names[blend_mode], 0))
    {
      for (int i = 0; i < 3; ++i)
      {
        if (igSelectable_Bool(blend_mode_names[i], i == blend_mode, 0, (struct ImVec2){ 0, 0 }))
        {
          set_model_animation_mixer_blend_mode(i);
        }
      }

      igEndCombo();
    }

    // Blend speed
    {
      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(200.0f);
      float blend_speed = get_model_animation_mixer_blend_speed();
      if (igSliderFloat("##AnimationMixerBlendSpeed", &blend_speed, 0.0f, 10.0f, "Blend Speed: %.2fx", 0))
      {
        set_model_animation_mixer_blend_speed(blend_speed);
      }
    }
  }

  // Channel row
  for (uint32_t channel_index = 0; channel_index < 4; ++channel_index)
  {
    igPushID_Int(channel_index);

    struct AEMAnimationChannel* channel = get_model_animation_channel(channel_index);

    // Animation dropdown
    igSetNextItemWidth(150.0f);
    if (igBeginCombo("##AnimationChannelDropdown",
                     (channel && animation_names) ? animation_names[channel->animation_index] : "", 0))
    {
      for (uint32_t animation_index = 0; animation_index < animation_name_count; ++animation_index)
      {
        if (igSelectable_Bool(animation_names[animation_index], channel->animation_index == animation_index, 0,
                              (struct ImVec2){ 0, 0 }))
        {
          channel->animation_index = animation_index;
          channel->time = 0.0f;
        }
      }

      igEndCombo();
    }

    // Play/pause button
    igSameLine(0.0f, -1.0f);
    if (channel && channel->is_playing)
    {
      if (igButton("Pause##AnimationChannelPause", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
      {
        channel->is_playing = false;
      }
    }
    else
    {
      if (igButton("Play##AnimationChannelPlay", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
      {
        channel->is_playing = true;
      }
    }

    // Stop button
    {
      igSameLine(0.0f, -1.0f);
      if (igButton("Stop##AnimationChannelStop", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }) && channel)
      {
        channel->is_playing = false;
        channel->time = 0.0f;
      }
    }

    // Loop checkbox
    {
      igSameLine(0.0f, -1.0f);
      bool looping = channel ? channel->is_looping : true;
      if (igCheckbox("Loop##AnimationChannelLoop", &looping) && channel)
      {
        channel->is_looping = looping;
      }
    }

    // Weight
    {
      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(200.0f);
      float weight = channel ? channel->weight : ((channel_index == 0) ? 1.0f : 0.0f);

      int percentage = (int)(weight * 100.0f);
      if (igSliderInt("##AnimationChannelWeight", &percentage, 0, 100, "Weight: %d%%", 0) && channel)
      {
        channel->weight = (float)percentage / 100.0f;
      }
    }

    // Blend button
    {
      igSameLine(0.0f, -1.0f);
      igBeginDisabled(channel ? (channel->weight >= 1.0f) : false);

      if (igButton("Blend##AnimationChannelBlend", (struct ImVec2){ PLAYBACK_BUTTON_WIDTH, 0.0f }))
      {
        blend_to_model_animation_channel(channel_index);
      }

      igEndDisabled();
    }

    // Playback speed
    {
      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(200.0f);
      float speed = channel ? channel->playback_speed : 1.0f;
      if (igSliderFloat("##AnimationChannelPlaybackSpeed", &speed, 0.0f, 10.0f, "Speed: %.2fx", 0) && channel)
      {
        channel->playback_speed = speed;
      }
    }

    // Timeline
    {
      float time = channel ? channel->time : 0.0f;
      const float duration = channel ? get_model_animation_duration(channel->animation_index) : 0.0f;

      const int playhead_m = (int)(time / 60.0f);
      const int playhead_s = (int)time;
      const int playhead_ms = (int)(fmod(time, 1.0f) * 1000.0f);

      const int total_m = (int)(duration / 60.0f);
      const int total_s = (int)duration;
      const int total_ms = (int)(fmod(duration, 1.0f) * 1000.0f);

      char buf[22];
      sprintf(buf, "%02d:%02d.%03d / %02d:%02d.%03d", playhead_m, playhead_s, playhead_ms, total_m, total_s, total_ms);

      igSameLine(0.0f, -1.0f);
      igSetNextItemWidth(-1.0f); // Stretch
      if (igSliderFloat("##AnimationChannelTimeline", &time, 0.0f, duration, buf, 0) && channel)
      {
        channel->time = time;
      }
    }

    igPopID();
  }

  igEndDisabled();

  igEnd();
}

void destroy_animation_mixer()
{
  free_animation_names();
}