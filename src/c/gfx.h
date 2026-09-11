#pragma once
#include <pebble.h>
#include "art_info.h"

// ============================================================
// 共通の描画ヘルパー
// ============================================================

// 画面の広さに応じたフォント
#define IS_LARGE_SCREEN (PBL_DISPLAY_WIDTH >= 200)
GFont gfx_font_small(void);       // 本文
GFont gfx_font_small_bold(void);  // 本文（太字）
GFont gfx_font_title(void);       // 見出し

// メニュー画面の配色（RPG風の青いウィンドウ）
#define THEME_BG GColorOxfordBlue
#define THEME_FG GColorWhite
#define THEME_SUB GColorPictonBlue
#define THEME_HI_BG GColorChromeYellow
#define THEME_HI_FG GColorBlack
#define THEME_HEADER_BG GColorDukeBlue

static inline GColor gfx_argb(uint8_t argb) { return (GColor){ .argb = argb }; }

// 文字マップを描画する（lut=NULL なら標準パレット）
void gfx_draw_charmap(GContext *ctx, const char *const *rows, int w, int h,
                      int x, int y, int scale, bool flip, const uint8_t *lut);

typedef enum {
  HERO_POSE_IDLE,
  HERO_POSE_WALK0,
  HERO_POSE_WALK1,
  HERO_POSE_ATTACK,
} HeroPose;

// 勇者を描く（装備中の武器・防具が見た目に反映される）
void gfx_draw_hero(GContext *ctx, int x, int y, HeroPose pose, int scale, bool flip);

// アイテムアイコン（16x16）。使う画面で acquire / release すること
void gfx_items_acquire(void);
void gfx_items_release(void);
void gfx_draw_item_icon(GContext *ctx, int item_id, int x, int y);

// ダンジョンごとの敵画像（24x24 x 2フレーム）のリソースID
uint32_t gfx_enemy_resource(int dungeon);

// 店主の画像（共有）
GBitmap *gfx_keeper_acquire(void);
void gfx_keeper_release(void);

// メニューの1行（アイコン＋タイトル＋サブテキスト＋右端の短い文字）
#define ROW_H (IS_LARGE_SCREEN ? 48 : 40)
typedef struct {
  int icon;            // アイテムアイコン番号（-1 でなし）
  GBitmap *bitmap;     // アイコンの代わりに描く 24x24 の画像（NULL 可）
  const char *title;
  const char *sub;
  const char *right;   // NULL 可
  bool dim;            // 使えない項目は暗く表示
  bool warn_sub;       // サブテキストを警告色（赤）にする
} RowSpec;
void gfx_draw_row(GContext *ctx, const Layer *cell, const RowSpec *row);
void gfx_setup_menu(MenuLayer *menu, Window *window);
// サブ行の見出し
void gfx_draw_header(GContext *ctx, const Layer *cell, const char *text);

// 小物
void gfx_draw_coin(GContext *ctx, int cx, int cy);
void gfx_draw_heart(GContext *ctx, int x, int y);
void gfx_draw_panel(GContext *ctx, GRect r, GColor fill, GColor border);
void gfx_draw_text(GContext *ctx, const char *text, GFont font, GRect r,
                   GTextAlignment align, GColor color);
void gfx_draw_shadow_text(GContext *ctx, const char *text, GFont font, GRect r,
                          GTextAlignment align, GColor color);
