#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "item_art.h"
#include "npc_header.h"

// ============================================================
// 酒場
//   その日に聞ける噂が3つ並ぶ。買うと「狙っている品」になり、そのダンジョンで出やすくなる
//   中身は選べないが、自分で聞き回る（図鑑から頼む）より安い
//   上に主人の顔とセリフ。長押しでその品の詳細
// ============================================================
static const char *const GREETINGS[] = {
  "Ale's warm, talk is cheap.",
  "Sit down. Folks have been talking.",
  "Travellers bring stories. I keep them.",
  "Word travels faster than you do.",
  "What'll it be, then?",
};
static const char *const THANKS[] = {
  "They say it's still down there.",
  "You didn't hear it from me.",
  "Bring it back and I'll pour you one.",
};

static Window *s_window;
static Layer *s_header;
static MenuLayer *s_menu;
static const char *s_speech;

static void say(const char *speech) {
  s_speech = speech;
  npc_header_say(s_header, speech);
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return TAVERN_OFFERS + 1;   // 噂3つと「聞き回る」
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "RUMOURS %ldG", (long)game_gold());
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

// 噂の品を見本として描くためのアイテム（図鑑のマスと同じ作り方）
static Item probe_item(int entry) {
  Item it = { 0 };
  it.base = (uint16_t)(entry + 1);
  it.flags = ITEM_FLAG_IDENTIFIED;
  it.seed = 15;
  if (entry < BASE_COUNT) {
    static const uint8_t TIER_LEVEL[TIER_COUNT] = { 8, 16, 25, 34, 42, 50 };
    it.ilvl = TIER_LEVEL[entry % TIER_COUNT];
    it.rarity = RARITY_NORMAL;
  } else {
    const SpecialDef *d = &g_specials[entry - BASE_COUNT];
    it.rarity = d->set_id ? RARITY_SET : RARITY_UNIQUE;
    it.ilvl = d->dungeon < DUNGEON_COUNT ? g_dungeons[d->dungeon].lvl_max : 30;
  }
  return it;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char name[40];
  static char sub[40];
  static Item it;   // 描き終わるまで残しておく
  RowSpec row = { .icon = -1 };

  if (index->row >= TAVERN_OFFERS) {
    row.icon = UI_ICON_CODEX;
    row.title = "Ask around";
    snprintf(sub, sizeof(sub), "Pick from the codex");
    row.sub = sub;
    gfx_draw_row(ctx, cell, &row);
    return;
  }

  int entry = game_tavern_offer(index->row);
  if (entry < 0) {
    row.title = "Nothing tonight";
    row.sub = "Come back tomorrow";
    row.dim = true;
    gfx_draw_row(ctx, cell, &row);
    return;
  }
  it = probe_item(entry);
  game_codex_name(entry, name, sizeof(name));
  static char place[24];
  game_codex_source(entry, place, sizeof(place));
  row.item = &it;
  row.title = name;
  row.tint_title = true;
  row.title_color = gfx_rarity_color(&it);
  if (game_rumour_has(entry)) {
    snprintf(sub, sizeof(sub), "Hunting: %s", place);
    row.tint_sub = true;
    row.sub_color = THEME_GOLD;
  } else {
    snprintf(sub, sizeof(sub), "%dG  %s", game_tavern_price(entry), place);
    row.warn_sub = game_gold() < game_tavern_price(entry);
  }
  row.sub = sub;
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->row >= TAVERN_OFFERS) {
    codex_window_push();
    return;
  }
  int entry = game_tavern_offer(index->row);
  if (entry < 0) return;
  if (game_rumour_has(entry)) {
    say("You're already after that one.");
    return;
  }
  switch (game_buy_tavern(index->row)) {
    case RUMOUR_OK:
      say(npc_pick(NPC_LINES(THANKS), s_speech));
      ui_state_changed();
      break;
    case RUMOUR_FULL:
      say("You're chasing enough already.");
      vibes_short_pulse();
      break;
    case RUMOUR_NO_GOLD:
      say("Coin first, story after.");
      vibes_short_pulse();
      break;
    case RUMOUR_NONE:
      vibes_short_pulse();
      break;
  }
  menu_layer_reload_data(menu);
}

// 長押しで品物の詳細（どんな品か確かめてから決める）
static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->row >= TAVERN_OFFERS) return;
  int entry = game_tavern_offer(index->row);
  if (entry < 0) return;
  Item it = probe_item(entry);
  item_window_push_copy(&it);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  s_speech = npc_pick(NPC_LINES(GREETINGS), NULL);
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_FACE_TAVERN, s_speech);
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

void tavern_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
