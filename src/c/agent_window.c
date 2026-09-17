#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "npc_header.h"

// ============================================================
// 回収代行業者
//   死んだ場所に残した装備と持ち物を、料金を払って取ってきてもらう
//   高い料金がかかるので、SELECT 2回（確認あり）で依頼する。BACK で取り消し
//   上にフードの冒険者の顔とセリフ、その下に場所・残り時間・料金
// ============================================================
static Window *s_window;
static Layer *s_header;
static Layer *s_body;
static bool s_confirm;   // 確認待ち

static void update_body(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  if (!game_drop_exists()) return;

  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 7, 3 * PX));
  GRect in = GRect(inset, 3 * PX, b.size.w - inset * 2, b.size.h - 3 * PX);
  GTextAlignment align = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);
  static char buf[96];
  int y = in.origin.y;

  const DungeonDef *d = &g_dungeons[game_drop_dungeon()];
  snprintf(buf, sizeof(buf), "%s F%d", d->name, game_drop_floor());
  y += gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H * 2), align, THEME_FG) * LINE_H;
  snprintf(buf, sizeof(buf), "%d items left behind", game_drop_item_count());
  y += gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H * 2), align, THEME_SUB) * LINE_H;
  int32_t left = game_drop_seconds_left();
  snprintf(buf, sizeof(buf), "Gone in %ldh %02ldm", (long)(left / 3600), (long)(left / 60 % 60));
  y += gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H), align, THEME_WARN) * LINE_H;
  snprintf(buf, sizeof(buf), "Fee %ldG (have %ldG)", (long)game_drop_fee(), (long)game_gold());
  y += gfx_text(ctx, buf, GRect(in.origin.x, y, in.size.w, LINE_H * 2), align,
                game_gold() < game_drop_fee() ? THEME_WARN : THEME_GOLD) * LINE_H;
  y += 2 * PX;
  gfx_text(ctx, s_confirm ? "SELECT=YES  BACK=NO" : "SELECT: Hire",
           GRect(in.origin.x, y, in.size.w, LINE_H * 2), align, s_confirm ? THEME_HI : THEME_SUB);
}

static void refresh(const char *speech) {
  npc_header_say(s_header, speech);
  layer_mark_dirty(s_body);
}

static void select_click(ClickRecognizerRef rec, void *ctx) {
  AgentResult check = game_agent_check();
  // 1回目は確認だけ。押し間違いでお金を払わないようにする
  if (!s_confirm && check == AGENT_OK) {
    s_confirm = true;
    refresh("Pay up and I'm off. Sure?");
    return;
  }
  s_confirm = false;
  switch (game_hire_agent()) {
    case AGENT_OK:
      ui_state_changed();
      ui_back_to_scene();
      return;
    case AGENT_NO_GOLD: refresh("No gold, no rescue."); break;
    case AGENT_NO_ROOM: refresh("Make room in your bag first."); break;
    case AGENT_NONE: refresh(game_drop_exists() ? "Not while you're out there." : "Nothing to fetch. Stay alive."); break;
  }
  vibes_short_pulse();
}

static void back_click(ClickRecognizerRef rec, void *ctx) {
  if (s_confirm) {   // 確認を取り消すだけ。画面は閉じない
    s_confirm = false;
    refresh("Changed your mind? Fine.");
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
  GRect b = layer_get_bounds(root);
  s_confirm = false;
  const char *greeting = game_drop_exists() ? "Lost your gear? I know the way." : "Nothing to fetch. Stay alive.";
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_FACE_AGENT, greeting);
  layer_add_child(root, s_header);
  s_body = layer_create(GRect(0, NPC_HEADER_H, b.size.w, b.size.h - NPC_HEADER_H));
  layer_set_update_proc(s_body, update_body);
  layer_add_child(root, s_body);
}

static void window_unload(Window *window) {
  layer_destroy(s_body);
  s_body = NULL;
  npc_header_destroy(s_header);
  s_header = NULL;
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
