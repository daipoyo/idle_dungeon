#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "npc_header.h"

// ============================================================
// 鑑定屋
//   持ち物の中の未鑑定の品を、お金を払って調べてもらう
//   巻物はダンジョンでも使えるので、持ち物画面（status_window）から使う
//   上に片眼鏡の老学者の顔とセリフ。鑑定した物のレア度でセリフが変わる
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static Layer *s_header;
static const char *s_speech;

static const char *const GREETINGS[] = {
  "Hmm? Let me see what you found.",
  "Every relic has a story.",
  "Bring it closer. My eyes are old.",
};

static void say(const char *speech) {
  s_speech = speech;
  npc_header_say(s_header, speech);
}

// 未鑑定の品だけを並べる。i 番目の持ち物の位置を返す
static int unident_index(int i) {
  int n = 0;
  for (int b = 0; b < BAG_SIZE; b++) {
    const Item *it = game_bag(b);
    if (!it || game_item_identified(it)) continue;
    if (n == i) return b;
    n++;
  }
  return -1;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  int n = game_unidentified_count();
  return n > 0 ? n : 1;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "APPRAISER %ldG", (long)game_gold());
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[32];
  RowSpec row = { .icon = -1 };
  int bag = unident_index(index->row);
  const Item *it = bag >= 0 ? game_bag(bag) : NULL;
  if (!it) {
    row.title = "All identified";
    row.sub = "Nothing to appraise";
    row.dim = true;
    gfx_draw_row(ctx, cell, &row);
    return;
  }
  static char name[32];
  int fee = game_identify_fee(it);
  row.item = it;
  game_item_short_name(it, name, sizeof(name));   // 鑑定するまで本当の名前は分からない
  row.title = name;
  row.tint_title = true;
  row.title_color = gfx_rarity_color(it);
  snprintf(sub, sizeof(sub), "Fee %dG", fee);
  row.sub = sub;
  row.warn_sub = game_gold() < fee;
  gfx_draw_row(ctx, cell, &row);
}

// 鑑定した物（持ち物の bag 番目）に合わせた一言
static void after_identify(IdentResult r, int bag) {
  switch (r) {
    case IDENT_OK: {
      const Item *it = game_bag(bag);
      int rarity = it ? it->rarity : RARITY_RARE;
      if (rarity == RARITY_UNIQUE) say("By the stars... a true legend!");
      else if (rarity == RARITY_SET) say("Part of a set! Find the rest.");
      else say("Aha. Quite a find, this one.");
      break;
    }
    case IDENT_NO_GOLD:
      say("My eyes aren't free, friend.");
      vibes_short_pulse();
      break;
    case IDENT_NO_SCROLL:
    case IDENT_NONE:
      return;
  }
  ui_state_changed();
  menu_layer_reload_data(s_menu);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  int bag = unident_index(index->row);
  if (bag < 0) return;
  // 鑑定すると並びから抜けるので、鑑定した物を先に控えておく
  after_identify(game_identify_with_gold(bag), bag);
}

static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  int bag = unident_index(index->row);
  if (bag >= 0) item_window_push(ITEM_AT_BAG, bag, false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  s_speech = npc_pick(NPC_LINES(GREETINGS), NULL);
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_FACE_APPRAISER, s_speech);
  layer_add_child(root, s_header);
  s_menu = menu_layer_create(GRect(0, NPC_HEADER_H, b.size.w, b.size.h - NPC_HEADER_H));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
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

void identify_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
