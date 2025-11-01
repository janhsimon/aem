#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct GLFWwindow GLFWwindow;

enum WindowMode
{
  WindowMode_Fullscreen,
  WindowMode_Windowed
};

bool load_window(enum WindowMode mode);

struct GLFWwindow* get_window();

void get_window_size(uint32_t* width, uint32_t* height);
void set_cursor_mode(bool first_person);

bool should_window_close();
void close_window();

void refresh_window();

void free_window();