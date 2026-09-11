#include <pebble.h>
#include "game.h"
#include "ui.h"

// ============================================================
// Idle Dungeon
//   町から出発 → ダンジョンを1回探索 → 自動で町に帰還して待機
//   画像は tools/gen_art.py で生成（resources/images/）
// ============================================================

void ui_state_changed(void) {
  menu_window_refresh();
  status_window_refresh();
}

void ui_back_to_scene(void) {
  Window *scene = scene_window_get();
  for (int i = 0; i < 8; i++) {
    Window *top = window_stack_get_top_window();
    if (!top || top == scene) break;
    window_stack_pop(true);
  }
}

static void init(void) {
  game_init();
  scene_window_push();
}

static void deinit(void) {
  game_save();
  scene_window_destroy();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
