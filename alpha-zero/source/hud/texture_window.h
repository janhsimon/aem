#pragma once

#include <stdbool.h>
#include <stdint.h>

bool begin_texture_window(const char* title,
                          bool menu_bar,
                          unsigned int window_index,
                          uint32_t screen_width,
                          uint32_t screen_height);
void add_texture_window_image(unsigned int texture_index, uint32_t screen_width, uint32_t screen_height);
void end_texture_window();