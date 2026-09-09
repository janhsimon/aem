#pragma once

struct ToolState;

void init_main_menu(struct ToolState* tool_state, void (*file_open_callback_)());

void update_main_menu();