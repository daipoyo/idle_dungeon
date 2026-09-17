#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// メイン画面
//   上から: 場所と所持金 / 場所の絵 / HP と進み具合 / ログ
//   ・歩数は数秒ごとに確認し、冒険を進める
//   ・UP/DOWN でログをさかのぼる、SELECT でメニュー
// ============================================================

#define UPDATE_MS 5000
#define KIND_TOWN DUNGEON_COUNT

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_timer;
static bool s_visible;
static int s_scroll;             // ログを何件さかのぼっているか

// 表示中の場所の絵だけ読み込む
static int s_art_kind = -1;
static GBitmap *s_bg, *s_far, *s_ground;

static const uint32_t BG_RES[6] = {
  RESOURCE_ID_IMG_BG_DUNGEON0, RESOURCE_ID_IMG_BG_DUNGEON1, RESOURCE_ID_IMG_BG_DUNGEON2,
  RESOURCE_ID_IMG_BG_DUNGEON3, RESOURCE_ID_IMG_BG_DUNGEON4, RESOURCE_ID_IMG_BG_DUNGEON5,
};
static const uint8_t BG_FILL[6] = {
  BG_DUNGEON0_FILL, BG_DUNGEON1_FILL, BG_DUNGEON2_FILL,
  BG_DUNGEON3_FILL, BG_DUNGEON4_FILL, BG_DUNGEON5_FILL,
};

typedef struct {
  int w, h;
  int inset;                     // 丸型の左右の余白
  int top_h;                     // 場所と所持金
  int art_top, art_bot;          // 絵
  int stat_top, stat_bot;        // HP と進み具合
  int log_top;                   // ログ
} Layout;

static Layout make_layout(GRect b) {
  Layout L;
  L.w = b.size.w;
  L.h = b.size.h;
#if defined(PBL_ROUND)
  L.inset = SNAP(L.w / 7);
  L.top_h = SNAP(L.h * 15 / 100);
  L.art_top = L.top_h;
  L.art_bot = SNAP(L.h * 42 / 100);
#else
  L.inset = 0;
  L.top_h = IS_LARGE_SCREEN ? 24 : 20;
  L.art_top = L.top_h;
  L.art_bot = L.art_top + (IS_LARGE_SCREEN ? 72 : 48);
#endif
  L.stat_top = L.art_bot;
  L.stat_bot = L.stat_top + LINE_H * 2 + 2 * PX;
  L.log_top = L.stat_bot;
  return L;
}

// ============================================================
// 画像の読み込み・解放
// ============================================================
static void destroy_bmp(GBitmap **b) {
  if (*b) {
    gbitmap_destroy(*b);
    *b = NULL;
  }
}

static void unload_art(void) {
  destroy_bmp(&s_far);
  destroy_bmp(&s_ground);
  destroy_bmp(&s_bg);
  s_art_kind = -1;
}

static void ensure_art(int kind) {
  if (kind == s_art_kind) return;
  unload_art();
  s_art_kind = kind;
  if (kind == KIND_TOWN) {
    s_bg = gbitmap_create_with_resource(RESOURCE_ID_IMG_BG_TOWN);
    return;
  }
  s_bg = gbitmap_create_with_resource(BG_RES[g_dungeons[kind].art]);
  s_far = gbitmap_create_as_sub_bitmap(s_bg, GRect(0, 0, BG_TILE_W, BG_FAR_H));
  s_ground = gbitmap_create_as_sub_bitmap(s_bg, GRect(0, BG_FAR_H, BG_TILE_W, BG_GROUND_H));
}

// ============================================================
// 描画
// ============================================================
static void draw_art(GContext *ctx, const Layout *L) {
  RunMode mode = game_run_mode();
  GRect area = GRect(0, L->art_top, L->w, L->art_bot - L->art_top);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  int hero_x;
  int foot_y = L->art_bot - PX;
  if (mode == RUN_NONE) {
    ensure_art(KIND_TOWN);
    graphics_context_set_fill_color(ctx, gfx_argb(BG_TOWN_FILL));
    graphics_fill_rect(ctx, area, 0, GCornerNone);
    graphics_draw_bitmap_in_rect(ctx, s_bg, GRect(SNAP((L->w - BG_TOWN_W) / 2), L->art_bot - BG_TOWN_H,
                                                   BG_TOWN_W, BG_TOWN_H));
    hero_x = SNAP(L->w / 2 - 40);
  } else {
    int d = game_run_dungeon();
    ensure_art(d);
    graphics_context_set_fill_color(ctx, gfx_argb(BG_FILL[g_dungeons[d].art]));
    graphics_fill_rect(ctx, area, 0, GCornerNone);
    int ground_top = L->art_bot - BG_GROUND_H;
    graphics_draw_bitmap_in_rect(ctx, s_far, GRect(0, ground_top - BG_FAR_H, L->w, BG_FAR_H));
    graphics_draw_bitmap_in_rect(ctx, s_ground, GRect(0, ground_top, L->w, BG_GROUND_H));
    hero_x = SNAP(L->w * 3 / 10);
  }
  // 帰り道では町の方（左）を向く
  gfx_draw_hero(ctx, hero_x, foot_y - HERO_H + 2 * PX, HERO_POSE_IDLE, PX, mode == RUN_RETURN);
}

static void draw_top_bar(GContext *ctx, const Layout *L) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, L->w, L->top_h), 0, GCornerNone);

  static char gold[16];
  snprintf(gold, sizeof(gold), "%ld", (long)game_gold());
  int ty = L->top_h - TEXT_H - 2 * PX;
  int gw = gfx_text_width(gold);
#if defined(PBL_ROUND)
  int gx = SNAP((L->w - gw + 6 * PX) / 2);
#else
  const char *place = game_run_mode() == RUN_NONE ? "Town" : g_dungeons[game_run_dungeon()].name;
  gfx_text(ctx, place, GRect(2 * PX, ty, L->w - gw - 14 * PX, LINE_H), GTextAlignmentLeft, THEME_FG);
  int gx = L->w - 2 * PX - gw;
#endif
  gfx_draw_coin(ctx, gx - 6 * PX, ty);
  gfx_text(ctx, gold, GRect(gx, ty, gw + PX, LINE_H), GTextAlignmentLeft, THEME_GOLD);
}

// HP バーと、進み具合（町ではレベルと持ち物）
static void draw_status(GContext *ctx, const Layout *L) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, L->stat_top, L->w, L->stat_bot - L->stat_top), 0, GCornerNone);
  int x = L->inset + 2 * PX;
  int w = L->w - L->inset * 2 - 4 * PX;
  int y = L->stat_top + PX;
  static char buf[32];

  // 1行目: HP の数値とバー
  int hp = game_hp(), max = game_max_hp();
  snprintf(buf, sizeof(buf), "HP %d/%d", hp, max);
  int tw = gfx_text_width("HP 000/000") + 2 * PX;
  gfx_text(ctx, buf, GRect(x, y, tw, LINE_H), GTextAlignmentLeft, THEME_FG);
  int bar_x = x + tw;
  int bar_w = SNAP(w - tw);
  graphics_context_set_fill_color(ctx, THEME_DIM);
  graphics_fill_rect(ctx, GRect(bar_x, y + PX, bar_w, 3 * PX), 0, GCornerNone);
  int filled = max > 0 ? SNAP(bar_w * hp / max) : 0;
  GColor hp_color = hp * 100 < max * 30 ? GColorRed : (hp * 100 < max * 60 ? GColorChromeYellow : GColorGreen);
  graphics_context_set_fill_color(ctx, hp_color);
  graphics_fill_rect(ctx, GRect(bar_x, y + PX, filled, 3 * PX), 0, GCornerNone);
  y += LINE_H;

  // 2行目: 左に今いる所、右に消耗品の数、ダンジョンではその間に奥行きのバー
  RunMode mode = game_run_mode();
  GColor label_color = THEME_SUB;
  if (mode == RUN_EXPLORE) {
    const DungeonDef *d = &g_dungeons[game_run_dungeon()];
    if (game_steps_to_next_floor() > 0) {
      snprintf(buf, sizeof(buf), "F%d/%d", game_floor(), d->floors);
    } else {
      snprintf(buf, sizeof(buf), "BOSS");
      label_color = GColorRed;
    }
  } else if (mode == RUN_RETURN) {
    snprintf(buf, sizeof(buf), "BACK");
    label_color = THEME_WARN;
  } else {
    snprintf(buf, sizeof(buf), "LV%d", game_level());
    label_color = THEME_GOLD;
  }
  int label_w = gfx_text_width(buf) + 3 * PX;
  gfx_text(ctx, buf, GRect(x, y, label_w, LINE_H), GTextAlignmentLeft, label_color);

  // 右端から消耗品の数（町では鑑定の巻物も）
  int items = mode == RUN_NONE ? 3 : 2;
  int counts_w = items * MINI_COUNT_W;
  int cx = x + w - counts_w;
  cx = gfx_draw_mini_count(ctx, MINI_POTION, game_potions(), cx, y, THEME_FG);
  cx = gfx_draw_mini_count(ctx, MINI_PORTAL, game_portals(), cx, y, THEME_FG);
  if (mode == RUN_NONE) gfx_draw_mini_count(ctx, MINI_IDENTIFY, game_identify_scrolls(), cx, y, THEME_FG);

  if (mode == RUN_NONE) return;
  // 奥行きのバー: 奥へ進むときは入口からの位置、帰り道は町までの残り。階の境目に切れ目
  int depth_x = x + label_w;
  int depth_w = SNAP(w - label_w - counts_w - 2 * PX);
  int total = game_run_total_steps();
  if (depth_w < 8 * PX || total <= 0) return;
  int pos = mode == RUN_RETURN ? game_return_left() : game_run_position();
  if (pos > total) pos = total;
  int by = y + PX;
  graphics_context_set_fill_color(ctx, THEME_DIM);
  graphics_fill_rect(ctx, GRect(depth_x, by, depth_w, 3 * PX), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, mode == RUN_RETURN ? THEME_WARN : GColorPictonBlue);
  graphics_fill_rect(ctx, GRect(depth_x, by, SNAP(depth_w * pos / total), 3 * PX), 0, GCornerNone);
  const DungeonDef *d = &g_dungeons[game_run_dungeon()];
  graphics_context_set_fill_color(ctx, GColorBlack);
  for (int f = 1; f < d->floors; f++) {
    int fx = depth_x + SNAP(depth_w * f / d->floors);
    if (d->floors * 2 * PX <= depth_w) graphics_fill_rect(ctx, GRect(fx, by, PX, 3 * PX), 0, GCornerNone);
  }
  // 最深部（ボス）の印
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(depth_x + depth_w - PX, by - PX, PX, 5 * PX), 0, GCornerNone);
}

// ログの色（最新の1件 / それより古いもの）
//   ダンジョン: 水色   町: 白   ボス撃破・レベルアップ・回収: 緑
//   死亡・消失・HP低下: 赤   まとめ: 黄
static GColor log_color(LogStyle style, bool latest) {
  switch (style) {
    case LOG_STYLE_DUNGEON: return latest ? GColorCeleste : GColorCadetBlue;
    case LOG_STYLE_GREAT: return latest ? GColorInchworm : GColorKellyGreen;
    case LOG_STYLE_DANGER: return latest ? GColorMelon : GColorRoseVale;
    case LOG_STYLE_SUMMARY: return latest ? GColorIcterine : GColorBrass;
    default: return latest ? GColorWhite : GColorLightGray;
  }
}

// ログ（新しい順）。s_scroll 件ぶんさかのぼって表示する
//   各行の左に種類の色の縦線を引く
static void draw_log(GContext *ctx, const Layout *L) {
#if defined(PBL_ROUND)
  GRect panel = GRect(0, L->log_top, L->w, L->h - L->log_top);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, panel, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, L->log_top, L->w, PX), 0, GCornerNone);
  GRect inner = GRect(L->inset, L->log_top + 2 * PX, L->w - L->inset * 2, L->h - L->log_top - 2 * PX);
  GTextAlignment align = GTextAlignmentCenter;
#else
  GRect panel = GRect(0, L->log_top, L->w, L->h - L->log_top);
  gfx_draw_window(ctx, panel);
  GRect inner = gfx_window_inner(panel);
  GTextAlignment align = GTextAlignmentLeft;
#endif
  int count = game_log_count();
  if (s_scroll >= count) s_scroll = count > 0 ? count - 1 : 0;
  int y = inner.origin.y;
  int bottom = inner.origin.y + inner.size.h;
  char buf[64];
  // 丸型は中央揃えなので縦線を引かず、色だけで区別する
  int bar_w = PBL_IF_ROUND_ELSE(0, 3 * PX);
  for (int i = s_scroll; i < count && y + LINE_H <= bottom; i++) {
    game_log_text(i, buf, sizeof(buf));
    GColor color = log_color(game_log_style(i), i == 0);
    GRect box = GRect(inner.origin.x + bar_w, y, inner.size.w - bar_w, bottom - y);
    int lines = gfx_text(ctx, buf, box, align, color);
    if (bar_w) {
      graphics_context_set_fill_color(ctx, color);
      graphics_fill_rect(ctx, GRect(inner.origin.x, y, PX, lines * LINE_H - PX), 0, GCornerNone);
    }
    y += lines * LINE_H + PX;
  }
  // さかのぼっているときは右上に印
  if (s_scroll > 0) {
    static char mark[16];
    snprintf(mark, sizeof(mark), "-%d", s_scroll);
    int mw = gfx_text_width(mark);
    gfx_text(ctx, mark, GRect(inner.origin.x + inner.size.w - mw, inner.origin.y, mw + PX, LINE_H),
             GTextAlignmentLeft, THEME_GOLD);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  Layout L = make_layout(layer_get_bounds(layer));
  draw_art(ctx, &L);
  draw_top_bar(ctx, &L);
  draw_status(ctx, &L);
  draw_log(ctx, &L);
}

// ============================================================
// 更新
// ============================================================
static void schedule_tick(void);

void scene_window_update(void) {
  if (game_update()) {
    s_scroll = 0;
    ui_state_changed();
  }
  if (s_visible && s_canvas) layer_mark_dirty(s_canvas);
}

static void tick(void *data) {
  s_timer = NULL;
  scene_window_update();
  schedule_tick();
}

static void schedule_tick(void) {
  if (s_timer) app_timer_cancel(s_timer);
  s_timer = s_visible ? app_timer_register(UPDATE_MS, tick, NULL) : NULL;
}

// ============================================================
// ボタン
// ============================================================
static void select_click(ClickRecognizerRef recognizer, void *context) {
  menu_window_push();
}

static void up_click(ClickRecognizerRef recognizer, void *context) {
  if (s_scroll + 1 < game_log_count()) {
    s_scroll++;
    layer_mark_dirty(s_canvas);
  }
}

static void down_click(ClickRecognizerRef recognizer, void *context) {
  if (s_scroll > 0) {
    s_scroll--;
    layer_mark_dirty(s_canvas);
  }
}

static void click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 150, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_click);
}

// ============================================================
// ウィンドウ
// ============================================================
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update_proc);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

static void window_appear(Window *window) {
  s_visible = true;
  s_scroll = 0;
  scene_window_update();
  schedule_tick();
}

static void window_disappear(Window *window) {
  // 他の画面を開いている間は画像を解放してメモリを空ける
  s_visible = false;
  unload_art();
  schedule_tick();
}

void scene_window_push(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
    .disappear = window_disappear,
  });
  window_stack_push(s_window, true);
}

void scene_window_destroy(void) {
  if (!s_window) return;   // 知らせで起きて、何も出さずに閉じたとき
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  unload_art();
  window_destroy(s_window);
  s_window = NULL;
}

Window *scene_window_get(void) {
  return s_window;
}
