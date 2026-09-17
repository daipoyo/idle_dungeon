#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "npc_header.h"

// ============================================================
// 保管庫
//   セクション0: 保管庫の中身 … 選ぶと持ち物へ
//   セクション1: 持ち物 … 選ぶと保管庫へ
//   保管庫の物は死んでも失わない
//   上に倉庫番の顔とセリフ
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static Layer *s_header;
static const char *s_speech;

static const char *const GREETINGS[] = {
  "Your things are safe with me!",
  "Dropping off or picking up?",
  "Nothing gets lost in my shed!",
};
static const char *const STORED[] = {
  "Tucked away safe!",
  "Got it. I'll keep an eye on it.",
};
static const char *const TAKEN[] = {
  "Here you go!",
  "Take good care of it!",
};

static void say(const char *speech) {
  s_speech = speech;
  npc_header_say(s_header, speech);
}

static uint16_t get_num_sections(MenuLayer *menu, void *data) {
  return 2;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  int n = section == 0 ? game_stash_count() : game_bag_count();
  return n > 0 ? n : 1;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[24];
  if (section == 0) snprintf(buf, sizeof(buf), "STASH %d/%d", game_stash_count(), STASH_SIZE);
  else snprintf(buf, sizeof(buf), "BAG %d/%d", game_bag_count(), BAG_SIZE);
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char name[40];
  static char sub[32];
  RowSpec row = { .icon = -1 };
  const Item *it = index->section == 0 ? game_stash(index->row) : game_bag(index->row);
  if (!it) {
    row.title = "Empty";
    row.sub = index->section == 0 ? "Keep gear safe here" : "Nothing to store";
    row.dim = true;
  } else {
    gfx_item_row(&row, it, name, sizeof(name), sub, sizeof(sub));
  }
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  bool take = index->section == 0;
  bool ok = take ? game_stash_take(index->row) : game_stash_put(index->row);
  if (!ok) {
    // 入れる先がいっぱい（空の行を選んだときは何も言わない）
    if (take ? game_stash(index->row) != NULL : game_bag(index->row) != NULL) {
      say(take ? "Your bag's full!" : "No more room here, sorry!");
      vibes_short_pulse();
    }
    return;
  }
  say(take ? npc_pick(NPC_LINES(TAKEN), s_speech) : npc_pick(NPC_LINES(STORED), s_speech));
  // 最後の行を動かしたときは、カーソルを残っている行へ
  int n = get_num_rows(menu, index->section, NULL);
  if (index->row >= n) {
    menu_layer_set_selected_index(menu, MenuIndex(index->section, n - 1), MenuRowAlignCenter, false);
  }
  menu_layer_reload_data(menu);
}

static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == 0 && game_stash(index->row)) item_window_push(ITEM_AT_STASH, index->row, false);
  if (index->section == 1 && game_bag(index->row)) item_window_push(ITEM_AT_BAG, index->row, false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  s_speech = npc_pick(NPC_LINES(GREETINGS), NULL);
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_FACE_STASH, s_speech);
  layer_add_child(root, s_header);
  s_menu = menu_layer_create(GRect(0, NPC_HEADER_H, b.size.w, b.size.h - NPC_HEADER_H));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = get_num_sections,
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
    .select_long_click = select_long_click,
  });
  gfx_setup_menu(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  s_menu = NULL;
  npc_header_destroy(s_header);
  s_header = NULL;
  window_destroy(window);
  s_window = NULL;
}

void stash_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
