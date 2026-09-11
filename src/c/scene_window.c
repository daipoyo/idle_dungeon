#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// メイン画面
//   ・探索中  : ダンジョンを横スクロールで進み、ときどき敵と戦う
//   ・探索終了: 宝箱（成功）または撤退（失敗）の演出のあと町へ
//   ・町      : 町の背景の前で勇者が待機
// ============================================================

#define KIND_TOWN 6

// 歩く速さと、敵との遭遇サイクル（ms）
#define WALK_SPEED 24          // px/秒
#define CYCLE_MS 6000
#define APPROACH_START 1600    // 敵が画面右から近づき始める
#define FIGHT_START 3200       // 戦闘開始
#define FIGHT_END 4700         // 戦闘終了（敵が消える）
#define POOF_END 5200          // 煙が消えて再び歩き出す
#define WALK_PER_CYCLE (FIGHT_START + (CYCLE_MS - POOF_END))
#define RESULT_MS 2800         // 帰還演出の長さ
#define FRAME_MS 150           // 探索中の描画間隔（約7fps）

static const uint32_t BG_RES[DUNGEON_COUNT] = {
  RESOURCE_ID_IMG_BG_DUNGEON0, RESOURCE_ID_IMG_BG_DUNGEON1, RESOURCE_ID_IMG_BG_DUNGEON2,
  RESOURCE_ID_IMG_BG_DUNGEON3, RESOURCE_ID_IMG_BG_DUNGEON4, RESOURCE_ID_IMG_BG_DUNGEON5,
};
static const uint8_t BG_FILL[DUNGEON_COUNT] = {
  BG_DUNGEON0_FILL, BG_DUNGEON1_FILL, BG_DUNGEON2_FILL,
  BG_DUNGEON3_FILL, BG_DUNGEON4_FILL, BG_DUNGEON5_FILL,
};
// 空中に浮かぶ敵（クラゲ・目玉）は少し高い位置に描く
static const int8_t ENEMY_LIFT[DUNGEON_COUNT] = { 0, 0, 0, 0, 8, 8 };

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_timer;
static bool s_visible;
static int64_t s_last_ms;
static uint32_t s_anim_ms;

// 背景などの画像（表示中の場所のものだけ読み込む）
static int s_art_kind = -1;
static GBitmap *s_bg, *s_far, *s_ground;
static GBitmap *s_enemy, *s_enemy_sub;
static GBitmap *s_chest, *s_chest_sub;

// 帰還演出
static bool s_result_active;
static int s_result_ms;
static RunResult s_result;
static bool s_items_held;

typedef struct {
  int w, h;
  int top_h, bot_h;
  int scene_top, scene_bot;
  int ground_top;
} Layout;

static Layout make_layout(GRect b) {
  Layout L;
  L.w = b.size.w;
  L.h = b.size.h;
#if defined(PBL_ROUND)
  L.top_h = L.h * 16 / 100;
  L.bot_h = L.h * 28 / 100;
#else
  L.top_h = IS_LARGE_SCREEN ? 26 : 20;
  L.bot_h = IS_LARGE_SCREEN ? 54 : 40;
#endif
  L.scene_top = L.top_h;
  L.scene_bot = L.h - L.bot_h;
  L.ground_top = L.scene_bot - BG_GROUND_H;
  return L;
}

static int64_t now_ms(void) {
  time_t t;
  uint16_t ms;
  time_ms(&t, &ms);
  return (int64_t)t * 1000 + ms;
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
  destroy_bmp(&s_enemy_sub);
  destroy_bmp(&s_enemy);
  destroy_bmp(&s_chest_sub);
  destroy_bmp(&s_chest);
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
  s_bg = gbitmap_create_with_resource(BG_RES[kind]);
  s_far = gbitmap_create_as_sub_bitmap(s_bg, GRect(0, 0, BG_TILE_W, BG_FAR_H));
  s_ground = gbitmap_create_as_sub_bitmap(s_bg, GRect(0, BG_FAR_H, BG_TILE_W, BG_GROUND_H));
  s_enemy = gbitmap_create_with_resource(gfx_enemy_resource(kind));
  s_enemy_sub = gbitmap_create_as_sub_bitmap(s_enemy, GRect(0, 0, 24, 24));
  s_chest = gbitmap_create_with_resource(RESOURCE_ID_IMG_CHEST);
  s_chest_sub = gbitmap_create_as_sub_bitmap(s_chest, GRect(0, 0, 16, 16));
}

static void hold_items(bool hold) {
  if (hold && !s_items_held) gfx_items_acquire();
  if (!hold && s_items_held) gfx_items_release();
  s_items_held = hold;
}

// ============================================================
// 探索の進み具合 → 歩いた距離・敵の状態
// ============================================================
static int enemy_cycles(int clear_ms) {
  int n = (clear_ms - 1000) / CYCLE_MS;
  return n < 0 ? 0 : n;
}

// 探索開始から p ms 経過した時点で、歩いていた時間の合計
static int walked_ms(int p, int clear_ms) {
  int n = enemy_cycles(clear_ms);
  int cyc = p / CYCLE_MS;
  int t = p % CYCLE_MS;
  int full_enemy = cyc < n ? cyc : n;
  int total = full_enemy * WALK_PER_CYCLE + (cyc - full_enemy) * CYCLE_MS;
  if (cyc < n) {
    if (t < FIGHT_START) total += t;
    else if (t < POOF_END) total += FIGHT_START;
    else total += FIGHT_START + (t - POOF_END);
  } else {
    total += t;
  }
  return total;
}

// ============================================================
// 描画
// ============================================================
static void draw_enemy(GContext *ctx, int frame, int x, int y) {
  if (!s_enemy_sub) return;
  gbitmap_set_bounds(s_enemy_sub, GRect(frame ? 24 : 0, 0, 24, 24));
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_enemy_sub, GRect(x, y, 24, 24));
}

static void draw_chest(GContext *ctx, bool open, int x, int y) {
  if (!s_chest_sub) return;
  gbitmap_set_bounds(s_chest_sub, GRect(open ? 16 : 0, 0, 16, 16));
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_chest_sub, GRect(x, y, 16, 16));
}

// 攻撃が当たったときの火花
static void draw_spark(GContext *ctx, int cx, int cy, int size) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(cx - size, cy), GPoint(cx + size, cy));
  graphics_draw_line(ctx, GPoint(cx, cy - size), GPoint(cx, cy + size));
  graphics_context_set_stroke_color(ctx, GColorIcterine);
  graphics_draw_line(ctx, GPoint(cx - size + 2, cy - size + 2), GPoint(cx + size - 2, cy + size - 2));
  graphics_draw_line(ctx, GPoint(cx - size + 2, cy + size - 2), GPoint(cx + size - 2, cy - size + 2));
}

// 敵を倒したときの煙
static void draw_poof(GContext *ctx, int cx, int cy, int t) {
  int r = 3 + t / 60;
  graphics_context_set_fill_color(ctx, (t < 250) ? GColorWhite : GColorLightGray);
  graphics_fill_circle(ctx, GPoint(cx - r / 2, cy), r / 2 + 1);
  graphics_fill_circle(ctx, GPoint(cx + r / 2, cy - 1), r / 2 + 1);
  graphics_fill_circle(ctx, GPoint(cx, cy - r / 2), r / 2 + 2);
}

static void draw_banner(GContext *ctx, const Layout *L, const char *text, GColor color) {
  gfx_draw_shadow_text(ctx, text, gfx_font_title(),
                       GRect(0, L->scene_top + 4, L->w, 30), GTextAlignmentCenter, color);
}

static void draw_dungeon_scene(GContext *ctx, const Layout *L, int dungeon, int p) {
  ensure_art(dungeon);
  int clear_ms = g_dungeons[dungeon].clear_time_sec * 1000;

  // 背景（遠景は地面の半分の速さでスクロール＝奥行き）
  graphics_context_set_fill_color(ctx, gfx_argb(BG_FILL[dungeon]));
  graphics_fill_rect(ctx, GRect(0, L->scene_top, L->w, L->scene_bot - L->scene_top), 0, GCornerNone);
  int scroll = s_result_active ? walked_ms(clear_ms, clear_ms) : walked_ms(p, clear_ms);
  scroll = scroll * WALK_SPEED / 1000;
  int far_off = (scroll / 2) % BG_TILE_W;
  int ground_off = scroll % BG_TILE_W;
  int far_top = L->ground_top - BG_FAR_H;
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, s_far, GRect(-far_off, far_top, L->w + far_off, BG_FAR_H));
  graphics_draw_bitmap_in_rect(ctx, s_ground, GRect(-ground_off, L->ground_top, L->w + ground_off, BG_GROUND_H));

  int hero_x = L->w * 3 / 10 - HERO_W / 2;
  int foot_y = L->ground_top + 4;
  int hero_y = foot_y - HERO_H;
  int enemy_y = foot_y - 24 - ENEMY_LIFT[dungeon];
  int contact_x = hero_x + 20;

  // ---- 帰還演出 ----
  if (s_result_active) {
    int t = s_result_ms;
    static char buf[24];
    if (s_result.success) {
      int chest_x = hero_x + 30;
      int chest_y = foot_y - 14;
      bool open = t >= 600;
      draw_chest(ctx, open, chest_x, chest_y);
      gfx_draw_hero(ctx, hero_x, hero_y, HERO_POSE_IDLE, 1, false);
      if (open) {
        int rise = (t - 600) / 30;
        if (rise > 16) rise = 16;
        if (s_result.item_id >= 0) {
          gfx_draw_item_icon(ctx, s_result.item_id, chest_x, chest_y - 6 - rise);
        }
        snprintf(buf, sizeof(buf), "+%dG", s_result.gold);
        gfx_draw_shadow_text(ctx, buf, gfx_font_small_bold(),
                             GRect(chest_x - 20, chest_y - 30 - rise, 56, 20),
                             GTextAlignmentCenter, GColorIcterine);
      }
      draw_banner(ctx, L, "CLEAR!", GColorIcterine);
    } else {
      if (t < 800) {
        if ((t / 100) % 2 == 0) gfx_draw_hero(ctx, hero_x, hero_y, HERO_POSE_IDLE, 1, false);
        draw_enemy(ctx, (t / 300) % 2, contact_x, enemy_y);
      } else {
        int run_x = hero_x - (t - 800) * 60 / 1000;
        HeroPose pose = ((t / 150) % 2) ? HERO_POSE_WALK0 : HERO_POSE_WALK1;
        gfx_draw_hero(ctx, run_x, hero_y, pose, 1, true);
        draw_enemy(ctx, (t / 300) % 2, contact_x, enemy_y);
      }
      draw_banner(ctx, L, "RETREAT...", GColorMelon);
    }
    return;
  }

  // ---- 探索中 ----
  int cyc = p / CYCLE_MS;
  int t = p % CYCLE_MS;
  bool enemy_cycle = cyc < enemy_cycles(clear_ms);
  HeroPose pose = ((p / 200) % 2) ? HERO_POSE_WALK0 : HERO_POSE_WALK1;

  if (enemy_cycle && t >= APPROACH_START && t < FIGHT_START) {
    int start_x = L->w + 4;
    int ex = start_x - (start_x - contact_x) * (t - APPROACH_START) / (FIGHT_START - APPROACH_START);
    draw_enemy(ctx, (t / 300) % 2, ex, enemy_y);
  } else if (enemy_cycle && t >= FIGHT_START && t < FIGHT_END) {
    int ft = t - FIGHT_START;
    bool swing = (ft / 250) % 2 == 0;
    pose = swing ? HERO_POSE_ATTACK : HERO_POSE_IDLE;
    int knock = swing ? 3 : 0;
    // 最後の一撃の前後は点滅
    if (ft < FIGHT_END - FIGHT_START - 300 || (ft / 60) % 2 == 0) {
      draw_enemy(ctx, (t / 300) % 2, contact_x + knock, enemy_y);
    }
    if (swing) draw_spark(ctx, contact_x + 4, foot_y - 12, 4);
  } else if (enemy_cycle && t >= FIGHT_END && t < POOF_END) {
    pose = HERO_POSE_IDLE;
    draw_poof(ctx, contact_x + 12, foot_y - 10, t - FIGHT_END);
  }
  gfx_draw_hero(ctx, hero_x, hero_y, pose, 1, false);

  if (p < 1800) {
    draw_banner(ctx, L, g_dungeons[dungeon].name, GColorWhite);
  }
}

static void draw_town_scene(GContext *ctx, const Layout *L) {
  ensure_art(KIND_TOWN);
  graphics_context_set_fill_color(ctx, gfx_argb(BG_TOWN_FILL));
  graphics_fill_rect(ctx, GRect(0, L->scene_top, L->w, L->scene_bot - L->scene_top), 0, GCornerNone);
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  graphics_draw_bitmap_in_rect(ctx, s_bg, GRect((L->w - BG_TOWN_W) / 2, L->scene_bot - BG_TOWN_H,
                                                 BG_TOWN_W, BG_TOWN_H));
  // お店の前でひと休み（ゆっくり呼吸）
  int bob = (s_anim_ms / 700) % 2;
  int hero_x = L->w / 2 - 34;
  int hero_y = L->ground_top + 4 - HERO_H + bob;
  gfx_draw_hero(ctx, hero_x, hero_y, HERO_POSE_IDLE, 1, false);
}

static void draw_top_bar(GContext *ctx, const Layout *L, const char *place) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, L->w, L->top_h), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_draw_line(ctx, GPoint(0, L->top_h - 1), GPoint(L->w, L->top_h - 1));

  static char gold[16];
  snprintf(gold, sizeof(gold), "%ld", (long)game_gold());
  GFont font = gfx_font_small_bold();
  int text_h = IS_LARGE_SCREEN ? 22 : 16;
  int ty = L->top_h - text_h - 3;
#if defined(PBL_ROUND)
  // 丸型：上端は狭いので所持金だけ中央に
  GSize sz = graphics_text_layout_get_content_size(gold, font, GRect(0, 0, L->w, 30),
                                                    GTextOverflowModeFill, GTextAlignmentLeft);
  int gx = (L->w - sz.w + 12) / 2;
  gfx_draw_coin(ctx, gx - 8, ty + text_h / 2 + 2);
  gfx_draw_text(ctx, gold, font, GRect(gx, ty - 1, sz.w + 4, text_h + 4), GTextAlignmentLeft, GColorWhite);
  (void)place;
#else
  gfx_draw_text(ctx, place, font, GRect(4, ty - 1, L->w - 60, text_h + 4), GTextAlignmentLeft, GColorChromeYellow);
  GSize sz = graphics_text_layout_get_content_size(gold, font, GRect(0, 0, L->w, 30),
                                                    GTextOverflowModeFill, GTextAlignmentLeft);
  int gx = L->w - 4 - sz.w;
  gfx_draw_coin(ctx, gx - 7, ty + text_h / 2 + 2);
  gfx_draw_text(ctx, gold, font, GRect(gx, ty - 1, sz.w + 2, text_h + 4), GTextAlignmentLeft, GColorWhite);
#endif
}

static void draw_bottom_panel(GContext *ctx, const Layout *L, bool exploring, int p, int clear_ms) {
  int y0 = L->scene_bot;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, y0, L->w, L->bot_h), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_draw_line(ctx, GPoint(0, y0), GPoint(L->w, y0));

  int inset = PBL_IF_ROUND_ELSE(L->w / 7, 4);
  GFont font = gfx_font_small();
  GTextAlignment align = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);
  int y = y0 + 3;

  if (exploring) {
    // 探索の進行バーと残り時間
    static char left[12];
    int remain = (clear_ms - p + 999) / 1000;
    snprintf(left, sizeof(left), "%ds", remain);
    int bar_x = inset;
    int bar_w = L->w - inset * 2 - 26;
    int filled = (clear_ms > 0) ? (bar_w - 2) * p / clear_ms : 0;
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_rect(ctx, GRect(bar_x, y + 3, bar_w, 7));
    graphics_context_set_fill_color(ctx, GColorChromeYellow);
    graphics_fill_rect(ctx, GRect(bar_x + 1, y + 4, filled, 5), 0, GCornerNone);
    gfx_draw_text(ctx, left, gfx_font_small_bold(), GRect(bar_x + bar_w, y - 4, 26, 18),
                  GTextAlignmentRight, GColorWhite);
    y += 12;
  }

  GRect box = GRect(inset, y, L->w - inset * 2, y0 + L->bot_h - y);
  gfx_draw_text(ctx, game_log(), font, box, align, GColorWhite);

  if (!exploring) {
    GSize sz = graphics_text_layout_get_content_size(game_log(), font, box,
                                                      GTextOverflowModeWordWrap, align);
    int line_h = IS_LARGE_SCREEN ? 20 : 16;
    if (sz.h <= line_h + 2) {
      gfx_draw_text(ctx, "SELECT: Menu", font, GRect(inset, y + line_h, box.size.w, line_h + 4),
                    align, GColorLightGray);
    }
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  Layout L = make_layout(layer_get_bounds(layer));
  bool exploring = !s_result_active && game_location() == LOC_DUNGEON;
  int p = game_progress_ms();

  const char *place = "Town";
  if (s_result_active) {
    draw_dungeon_scene(ctx, &L, s_result.dungeon, p);
    place = g_dungeons[s_result.dungeon].name;
  } else if (exploring) {
    draw_dungeon_scene(ctx, &L, game_current_dungeon(), p);
    place = g_dungeons[game_current_dungeon()].name;
  } else {
    draw_town_scene(ctx, &L);
  }
  draw_top_bar(ctx, &L, place);
  draw_bottom_panel(ctx, &L, exploring, p, game_clear_ms());
}

// ============================================================
// タイマー
// ============================================================
static void schedule_tick(void);

static void tick(void *data) {
  s_timer = NULL;
  int64_t now = now_ms();
  int dt = (int)(now - s_last_ms);
  if (dt < 0) dt = 0;
  if (dt > 1000) dt = 1000;
  s_last_ms = now;
  s_anim_ms += dt;

  RunResult r;
  if (game_update(&r)) {
    if (s_visible) {
      s_result = r;
      s_result_active = true;
      s_result_ms = 0;
      hold_items(true);
    }
    ui_state_changed();
  }
  if (s_result_active) {
    s_result_ms += dt;
    if (s_result_ms >= RESULT_MS) {
      s_result_active = false;
      hold_items(false);
    }
  }
  if (s_visible && s_canvas) layer_mark_dirty(s_canvas);
  schedule_tick();
}

static void schedule_tick(void) {
  if (s_timer) app_timer_cancel(s_timer);
  int ms = 1000;
  if (s_visible) {
    ms = (s_result_active || game_location() == LOC_DUNGEON) ? FRAME_MS : 350;
  }
  s_timer = app_timer_register(ms, tick, NULL);
}

// ============================================================
// ウィンドウ
// ============================================================
static void select_click(ClickRecognizerRef recognizer, void *context) {
  menu_window_push();
}

static void click_config(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

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
  s_last_ms = now_ms();
  schedule_tick();
  layer_mark_dirty(s_canvas);
}

static void window_disappear(Window *window) {
  // 他の画面を開いている間は画像を解放してメモリを空ける
  s_visible = false;
  s_result_active = false;
  hold_items(false);
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
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  unload_art();
  hold_items(false);
  window_destroy(s_window);
  s_window = NULL;
}

Window *scene_window_get(void) {
  return s_window;
}
