#include "enemy_manager.h"

#include "enemy.h"

#include "model_manager.h"

#include <assert.h>
#include <stdlib.h>

static const struct Preferences* preferences = NULL;
static struct ModelRenderInfo* render_info = NULL;

static uint32_t enemy_count = 0;
static struct Enemy* enemies = NULL;

bool load_enemy_manager(const struct Preferences* preferences_, uint32_t enemy_count_)
{
  preferences = preferences_;
  enemy_count = enemy_count_;

  render_info = load_model("models/soldier.aem");
  if (!render_info)
  {
    return false;
  }

  // Initialize enemies
  enemies = malloc(sizeof(*enemies) * enemy_count);
  assert(enemies);
  for (uint32_t enemy_index = 0; enemy_index < enemy_count; ++enemy_index)
  {
    struct Enemy* enemy = &enemies[enemy_index];
    if (!load_enemy(enemy, preferences, render_info->model))
    {
      return false;
    }
  }

  return true;
}

void update_enemy_manager(float delta_time)
{
  for (uint32_t enemy_index = 0; enemy_index < enemy_count; ++enemy_index)
  {
    struct Enemy* enemy = &enemies[enemy_index];
    update_enemy(preferences, enemy, render_info->model, delta_time);
  }
}

struct ModelRenderInfo* get_enemy_render_info()
{
  return render_info;
}

uint32_t get_enemy_count()
{
  return enemy_count;
}

struct Enemy* get_enemy(uint32_t enemy_index)
{
  return &enemies[enemy_index];
}

void free_enemy_manager()
{
  for (uint32_t enemy_index = 0; enemy_index < enemy_count; ++enemy_index)
  {
    struct Enemy* enemy = &enemies[enemy_index];
    free_enemy(enemy);
  }

  free(enemies);
}