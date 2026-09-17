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

// ============================================================
// 出発の知らせ
//   設定した時刻にアプリを起こす（Wakeup）。町にいたらダンジョン選択を開いて振動で知らせる。
//   冒険中なら、閉じていた間の歩数を反映するだけで、何も出さずに閉じる。
//   予約はアプリを閉じるたびに、次の同じ時刻で入れ直す。
// ============================================================
#define REMIND_COOKIE 1

static void schedule_reminder(void) {
  wakeup_cancel_all();
  int hour = game_remind_hour();
  if (!hour) return;
  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  time_t at = now - tm->tm_hour * SECONDS_PER_HOUR - tm->tm_min * SECONDS_PER_MINUTE - tm->tm_sec +
              hour * SECONDS_PER_HOUR;
  if (at < now + SECONDS_PER_MINUTE) at += SECONDS_PER_DAY;
  // ほかのアプリの予約と時刻が近いと断られるので、少しずつずらして試す
  for (int i = 0; i < 5; i++) {
    if (wakeup_schedule(at + i * 2 * SECONDS_PER_MINUTE, REMIND_COOKIE, false) >= 0) break;
  }
}

static void init(void) {
  game_init();
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    game_update();   // 閉じていた間に町へ着いているかもしれない
    if (game_run_mode() != RUN_NONE) return;
    scene_window_push();
    dungeon_window_push();
    vibes_double_pulse();
    return;
  }
  scene_window_push();
}

static void deinit(void) {
  game_save();
  schedule_reminder();
  scene_window_destroy();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
