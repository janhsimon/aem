#pragma once

typedef struct cgltf_animation_channel cgltf_animation_channel;

void determine_interpolated_values_for_keyframe_at(const cgltf_animation_channel* channel,
                                                   float time,
                                                   int element_count,
                                                   float out_values[4]);
