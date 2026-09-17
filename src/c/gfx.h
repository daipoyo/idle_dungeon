#pragma once
#include <pebble.h>
#include "art_info.h"
#include "game.h"

// ============================================================
// 共通の描画ヘルパー（ゲームボーイカラー風）
//   絵も文字も「1ドット = PX x PX ピクセル」で描く。
// ============================================================

#define IS_LARGE_SCREEN (PBL_DISPLAY_WIDTH >= 200)

// 座標をドットの格子に合わせる（PX は 2 のべき乗）
#define SNAP(v) ((v) & ~(PX - 1))

// メニュー画面の配色（黒いウィンドウに白い文字）
#define THEME_BG GColorBlack
#define THEME_FG GColorWhite
#define THEME_SUB GColorLightGray
#define THEME_DIM GColorDarkGray
#define THEME_HI GColorIcterine    // 選択中の行の文字
#define THEME_WARN GColorMelon
#define THEME_GOLD GColorIcterine

static inline GColor gfx_argb(uint8_t argb) { return (GColor){ .argb = argb }; }

// ---- ドットフォント ----
#define TEXT_H (5 * PX)            // 文字の高さ
#define LINE_H (6 * PX)            // 行の高さ
// 1行の幅（ピクセル）
int gfx_text_width(const char *text);
// 枠の幅で折り返したときの行数
int gfx_text_lines(const char *text, int width);
// 枠の中に折り返して描く（枠の高さに入らない行は描かない）。描いた行数を返す
int gfx_text(GContext *ctx, const char *text, GRect box, GTextAlignment align, GColor color);
// 縁取り付きの文字（背景の上に出す見出し用）
void gfx_text_outlined(GContext *ctx, const char *text, GRect box, GTextAlignment align,
                       GColor color, GColor outline);

// 文字マップを描画する（lut=NULL なら標準パレット）。scale は1ドットのピクセル数
void gfx_draw_charmap(GContext *ctx, const char *const *rows, int w, int h,
                      int x, int y, int scale, bool flip, const uint8_t *lut);

typedef enum {
  HERO_POSE_IDLE,
  HERO_POSE_WALK0,
  HERO_POSE_WALK1,
  HERO_POSE_ATTACK,
} HeroPose;

// 勇者の大きさ（PX で描いたときのピクセル数）
#define HERO_W (HERO_MAP_W * PX)
#define HERO_H (HERO_MAP_H * PX)
// 勇者を描く（装備中の武器・防具が見た目に反映される）。scale は1ドットのピクセル数
void gfx_draw_hero(GContext *ctx, int x, int y, HeroPose pose, int scale, bool flip);

// アイテムアイコン（ICON_SIZE 四方）。使う画面で acquire / release すること
void gfx_items_acquire(void);
void gfx_items_release(void);
void gfx_draw_item_icon(GContext *ctx, int icon, int x, int y);
// 図鑑アイテムの絵（ITEM_ICON_SIZE 四方）。素材の段階で色が変わる。
// 未鑑定の固有・セット装備は、正体が分からないよう元の形の絵で描く
#define ITEM_ICON_SIZE (12 * PX)
void gfx_draw_item(GContext *ctx, const Item *it, int x, int y);
// 固有・セット装備の飾り: アイコンの四隅の枠（金・緑）と、角で瞬くきらめき。
//   きらめきは固有装備と、3部位そろったセット装備だけ。frame はアニメーションのコマ
bool gfx_item_has_glow(const Item *it);
void gfx_draw_item_glow(GContext *ctx, const Item *it, int x, int y, int frame);
void gfx_set_glow_frame(int frame);   // 一覧の行（gfx_draw_row）で使うコマ
// 十字の星のきらめき（中心 cx, cy）。phase で大きさが 点→小→大→小 と脈打つ
void gfx_draw_twinkle(GContext *ctx, int cx, int cy, int phase, GColor color);
// 性能の短い文字列（"ATK+12 HP+5"）
void gfx_item_stat_text(const Item *it, char *buf, size_t size);
// レア度の色（白・青・黄・緑・金）
GColor gfx_rarity_color(const Item *it);

// 敵画像（ENEMY_SIZE 四方 x 2フレーム）のリソースID。番号は背景の絵と同じ
uint32_t gfx_enemy_resource(int dungeon);

// 店主の顔の画像（共有）
GBitmap *gfx_keeper_acquire(void);
void gfx_keeper_release(void);

// メニューの1行（カーソル＋アイコン＋タイトル＋サブテキスト＋右端の短い文字）
#define ROW_H (IS_LARGE_SCREEN ? 48 : 40)
typedef struct {
  int icon;            // メニュー用の小さいアイコン番号（-1 でなし）
  const Item *item;    // 図鑑アイテムの絵（icon より優先。NULL 可）
  GBitmap *bitmap;     // アイコンの代わりに描く ENEMY_SIZE 四方の画像（NULL 可）
  const char *title;
  const char *sub;
  const char *right;   // NULL 可
  bool dim;            // 使えない項目は暗く表示
  bool warn_sub;       // サブテキストを警告色にする
  bool tint_title;     // タイトルをレア度の色で描く
  GColor title_color;
  bool tint_right;     // 右端の文字を right_color で描く（セットの部位数など）
  GColor right_color;
} RowSpec;
void gfx_draw_row(GContext *ctx, const Layer *cell, const RowSpec *row);
// アイテム1個分の行（アイコン・レア度の色の名前・性能。未鑑定なら警告色）を row に詰める。
// name / sub は呼び出し側のバッファ（描き終わるまで生きていること）
void gfx_item_row(RowSpec *row, const Item *it, char *name, size_t name_size, char *sub,
                  size_t sub_size);
void gfx_setup_menu(MenuLayer *menu, Window *window);
// セクションの見出し
#define MENU_HEADER_H 18
void gfx_draw_header(GContext *ctx, const Layer *cell, const char *text);

// 小物
void gfx_draw_coin(GContext *ctx, int x, int y);         // 左上座標。5ドット四方
void gfx_draw_heart(GContext *ctx, int x, int y);        // 左上座標。文字1つ分
void gfx_draw_cursor(GContext *ctx, int x, int y, GColor color);  // 右向き三角
void gfx_draw_spark(GContext *ctx, int cx, int cy);
void gfx_draw_poof(GContext *ctx, int cx, int cy, int frame);
// ウィンドウ枠（黒地に白い線）。_color は線の色を変えたもの
void gfx_draw_window(GContext *ctx, GRect r);
void gfx_draw_window_color(GContext *ctx, GRect r, GColor line);
// ウィンドウ枠の内側（文字を置ける範囲）
GRect gfx_window_inner(GRect r);
