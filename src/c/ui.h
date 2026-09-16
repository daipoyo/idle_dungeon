#pragma once
#include <pebble.h>

// ============================================================
// 各画面の入口
// ============================================================
void scene_window_push(void);     // メイン画面（町／ダンジョン）
void scene_window_destroy(void);
Window *scene_window_get(void);
void scene_window_update(void);   // 歩数を確認して表示を更新する

void menu_window_push(void);      // メニュー
void menu_window_refresh(void);

void dungeon_window_push(void);   // ダンジョン選択

void shop_window_push(void);      // お店

void status_window_push(void);    // ステータス・装備・持ち物
void status_window_refresh(void);

void agent_window_push(void);     // 回収代行業者

void settings_window_push(void);  // 設定

// 冒険の状態が変わったときに、開いている画面を更新する
void ui_state_changed(void);
// メイン画面まで戻る
void ui_back_to_scene(void);
