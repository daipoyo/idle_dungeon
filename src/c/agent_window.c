#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 回収代行業者
//   死んだ場所に残した装備と持ち物を、料金を払って取ってきてもらう
//   高い料金がかかるので、SELECT 2回（確認あり）で依頼する。BACK で取り消し
// ============================================================
static Window *s_window;
static Layer *s_layer;
static const char *s_message;
static bool s_confirm;   // 確認待ち

static void update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 7, 0));
  GRect panel = GRect(inset, PBL_IF_ROUND_ELSE(SNAP(b.size.h / 8), 0), b.size.w - inset * 2,
                      b.size.h - PBL_IF_ROUND_ELSE(SNAP(b.size.h / 4), 0));
  gfx_draw_window(ctx, panel);
  GRect in = gfx_window_inner(panel);
  GTextAlignment align = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);

  int y = in.origin.y;
  gfx_text(ctx, "RETRIEVAL AGENT", GRect(in.origin.x, y, in.size.w, LINE_H), align, THEME_GOLD);
  y += LINE_H + 2 * PX;

  static char buf[96];
  if (!game_drop_exists()) {
    gfx_text(ctx, "Nothing to fetch. Stay alive out there.",
             GRect(in.origin.x, y, in.size.w, in.origin.y + in.size.h - y), align, THEME_FG);
    return;
  }
  const DungeonDef *d = &g_dungeons[game_drop_dungeon()];
  int32_t left = game_drop_seconds_left();
  snprintf(buf, sizeof(buf), "Your gear lies in %s F%d. %d items.", d->name, game_drop_floor(),
           game_drop_item_count());
  int lines = gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H * 3), align, THEME_FG);
  y += lines * LINE_H + PX;
  snprintf(buf, sizeof(buf), "Gone in %ldh %02ldm", (long)(left / 3600), (long)(left / 60 % 60));
  gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H), align, THEME_WARN);
  y += LINE_H;
  snprintf(buf, sizeof(buf), "Fee %ldG (have %ldG)", (long)game_drop_fee(), (long)game_gold());
  lines = gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H * 2), align,
                   game_gold() < game_drop_fee() ? THEME_WARN : THEME_GOLD);
  y += lines * LINE_H + PX;

  static char prompt[48];
  const char *msg = "SELECT: Hire";
  GColor color = THEME_SUB;
  if (s_message) {
    msg = s_message;
    color = THEME_HI;
  } else if (s_confirm) {
    snprintf(prompt, sizeof(prompt), "Pay %ldG? SELECT=YES BACK=NO", (long)game_drop_fee());
    msg = prompt;
    color = THEME_WARN;
  }
  gfx_text(ctx, msg, GRect(in.origin.x, y, in.size.w, in.origin.y + in.size.h - y), align, color);
}

static void select_click(ClickRecognizerRef rec, void *ctx) {
  // 1回目は確認だけ。押し間違いでお金を払わないようにする
  if (!s_confirm && game_agent_check() == AGENT_OK) {
    s_confirm = true;
    s_message = NULL;
    layer_mark_dirty(s_layer);
    return;
  }
  s_confirm = false;
  switch (game_hire_agent()) {
    case AGENT_OK:
      s_message = NULL;
      ui_state_changed();
      ui_back_to_scene();
      return;
    case AGENT_NO_GOLD: s_message = "Not enough gold."; break;
    case AGENT_NO_ROOM: s_message = "Bag has no room. Sell some items."; break;
    case AGENT_NONE: s_message = "Can't do that now."; break;
  }
  vibes_short_pulse();
  layer_mark_dirty(s_layer);
}

static void back_click(ClickRecognizerRef rec, void *ctx) {
  if (s_confirm) {   // 確認を取り消すだけ。画面は閉じない
    s_confirm = false;
    s_message = NULL;
    layer_mark_dirty(s_layer);
    return;
  }
  window_stack_pop(true);
}

static void click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(root, s_layer);
  s_message = NULL;
  s_confirm = false;
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
  s_layer = NULL;
  window_destroy(window);
  s_window = NULL;
}

void agent_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
