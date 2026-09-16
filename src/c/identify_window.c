#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 鑑定屋
//   持ち物の中の未鑑定の品を、お金を払って調べてもらう
//   巻物はダンジョンでも使えるので、持ち物画面（status_window）から使う
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static const char *s_message;

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
  if (s_message) {
    gfx_draw_header(ctx, cell, s_message);
    return;
  }
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
  row.icon = game_item_shape(it)->icon;
  game_item_short_name(it, name, sizeof(name));   // 鑑定するまで本当の名前は分からない
  row.title = name;
  row.tint_title = true;
  row.title_color = gfx_rarity_color(it);
  snprintf(sub, sizeof(sub), "Fee %dG", fee);
  row.sub = sub;
  row.warn_sub = game_gold() < fee;
  gfx_draw_row(ctx, cell, &row);
}

static void after_identify(IdentResult r) {
  switch (r) {
    case IDENT_OK: s_message = NULL; break;
    case IDENT_NO_GOLD: s_message = "NOT ENOUGH GOLD"; break;
    case IDENT_NO_SCROLL: s_message = "NO SCROLLS"; break;
    case IDENT_NONE: return;
  }
  if (r != IDENT_OK) vibes_short_pulse();
  ui_state_changed();
  menu_layer_reload_data(s_menu);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  int bag = unident_index(index->row);
  if (bag < 0) return;
  after_identify(game_identify_with_gold(bag));
}

static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  int bag = unident_index(index->row);
  if (bag >= 0) item_window_push(ITEM_AT_BAG, bag, false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  gfx_items_acquire();
  s_message = NULL;
  s_menu = menu_layer_create(layer_get_bounds(root));
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
  gfx_items_release();
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
