#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "item_art.h"
#include "npc_header.h"

// ============================================================
// お店
//   上に女主人の顔とセリフ、その下に一覧:
//     BUY  … ポーション、帰還の巻物、鑑定の巻物、装備（勇者のレベルに合わせた品）
//     SELL … 持ち物
//   SELECT で買う・売る。長押しで品物の詳細
// ============================================================
#define SECTION_BUY 0
#define SECTION_SELL 1

#define BUY_POTION 0
#define BUY_PORTAL 1
#define BUY_IDENT 2
#define BUY_GEAR_FIRST 3

// ドットフォントは1行に入る文字数が少ないので、セリフは短めにしてある
static const char *const GREETINGS[] = {
  "Welcome, darling~",
  "Oh, it's you again~",
  "Hi handsome~ Need something?",
  "You look strong today~",
  "Come back safe, okay?",
  "I saved the best for you~",
  "Bring me something shiny?",
};
static const char *const THANKS[] = {
  "Great choice, darling~",
  "It suits you~",
  "A man of taste~",
};
static const char *const SOLD[] = {
  "Ooh, I'll take that~",
  "Pleasure doing business~",
  "Shiny! Thank you~",
};

static Window *s_window;
static Layer *s_header;
static MenuLayer *s_menu;
static const char *s_speech;

static void say(const char *speech) {
  s_speech = speech;
  npc_header_say(s_header, speech);
}

static int sell_rows(void) {
  int n = game_bag_count();
  return n > 0 ? n : 1;
}

static uint16_t get_num_sections(MenuLayer *menu, void *data) {
  return 2;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return section == SECTION_BUY ? BUY_GEAR_FIRST + SHOP_GEAR_COUNT : sell_rows();
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  if (section == SECTION_BUY) snprintf(buf, sizeof(buf), "BUY %ldG", (long)game_gold());
  else snprintf(buf, sizeof(buf), "SELL BAG %d/%d", game_bag_count(), BAG_SIZE);
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_buy_row(GContext *ctx, const Layer *cell, int r) {
  static char sub[32];
  static char right[8];
  RowSpec row = { .icon = -1 };
  right[0] = '\0';
  if (r == BUY_POTION) {
    row.icon = UI_ICON_POTION;
    row.title = "Potion";
    snprintf(sub, sizeof(sub), "%dG Heal 50%%", POTION_PRICE);
    snprintf(right, sizeof(right), "%d/%d", game_potions(), MAX_POTIONS);
    row.warn_sub = game_gold() < POTION_PRICE;
  } else if (r == BUY_PORTAL) {
    row.icon = UI_ICON_PORTAL_SCROLL;
    row.title = "Portal Scroll";
    snprintf(sub, sizeof(sub), "%dG To town", PORTAL_PRICE);
    snprintf(right, sizeof(right), "%d/%d", game_portals(), MAX_PORTALS);
    row.warn_sub = game_gold() < PORTAL_PRICE;
  } else if (r == BUY_IDENT) {
    row.icon = UI_ICON_IDENTIFY_SCROLL;
    row.title = "Identify Scroll";
    snprintf(sub, sizeof(sub), "%dG Reveals gear", IDENT_SCROLL_PRICE);
    snprintf(right, sizeof(right), "%d/%d", game_identify_scrolls(), MAX_IDENT_SCROLLS);
    row.warn_sub = game_gold() < IDENT_SCROLL_PRICE;
  } else {
    int i = r - BUY_GEAR_FIRST;
    static char name[32];
    static Item it;   // 描き終わるまで残しておく
    it = game_shop_gear(i);
    char stats[24];
    gfx_item_stat_text(&it, stats, sizeof(stats));
    int cost = game_shop_gear_cost(i);
    row.item = &it;
    game_item_short_name(&it, name, sizeof(name));
    row.title = name;
    snprintf(sub, sizeof(sub), "%dG %s", cost, stats);
    row.warn_sub = game_gold() < cost;
  }
  row.sub = sub;
  if (right[0]) row.right = right;
  gfx_draw_row(ctx, cell, &row);
}

static void draw_sell_row(GContext *ctx, const Layer *cell, int r) {
  static char name[40];
  static char stats[24];
  static char sub[32];
  RowSpec row = { .icon = -1 };
  const Item *it = game_bag(r);
  if (!it) {
    row.title = "Nothing to sell";
    row.sub = "Go find loot!";
    row.dim = true;
  } else {
    gfx_item_row(&row, it, name, sizeof(name), stats, sizeof(stats));
    snprintf(sub, sizeof(sub), game_item_identified(it) ? "Sell %dG" : "Sell %dG (unident.)",
             game_item_price(it));
    row.sub = sub;
    row.warn_sub = false;
  }
  gfx_draw_row(ctx, cell, &row);
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  if (index->section == SECTION_BUY) draw_buy_row(ctx, cell, index->row);
  else draw_sell_row(ctx, cell, index->row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == SECTION_BUY) {
    int r = index->row;
    BuyResult result;
    if (r == BUY_POTION) result = game_buy_potion();
    else if (r == BUY_PORTAL) result = game_buy_portal();
    else if (r == BUY_IDENT) result = game_buy_identify();
    else result = game_buy_gear(r - BUY_GEAR_FIRST);
    switch (result) {
      case BUY_OK: say(npc_pick(NPC_LINES(THANKS), s_speech)); break;
      case BUY_NO_GOLD: say("Not enough gold, sweetie."); vibes_short_pulse(); break;
      case BUY_FULL:
        say(r < BUY_GEAR_FIRST ? "You can't carry more, dear." : "Your bag is full, dear.");
        vibes_short_pulse();
        break;
    }
  } else {
    if (!game_sell_bag(index->row)) return;
    say(npc_pick(NPC_LINES(SOLD), s_speech));
    int n = sell_rows();
    if (index->row >= n) {
      menu_layer_set_selected_index(menu, MenuIndex(SECTION_SELL, n - 1), MenuRowAlignCenter, false);
    }
  }
  menu_layer_reload_data(menu);
}

// 長押しで品物の詳細（買う前・売る前に中身を確かめる）
static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == SECTION_BUY) {
    if (index->row < BUY_GEAR_FIRST) return;
    Item it = game_shop_gear(index->row - BUY_GEAR_FIRST);
    item_window_push_copy(&it);
  } else if (game_bag(index->row)) {
    item_window_push(ITEM_AT_BAG, index->row, false);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  s_speech = npc_pick(NPC_LINES(GREETINGS), NULL);
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_KEEPER_FACE, s_speech);
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

void shop_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
