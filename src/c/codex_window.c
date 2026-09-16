#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 図鑑
//   一度手に入れた物だけ名前と絵が出る。まだの物は "???"
//   （セット・固有装備は鑑定して名前が分かったときに載る）
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return game_codex_size();
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[24];
  snprintf(buf, sizeof(buf), "CODEX %d/%d", game_codex_seen_count(), game_codex_size());
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static GColor rarity_color(int rarity) {
  Item probe = { .rarity = (uint8_t)rarity };
  return gfx_rarity_color(&probe);
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[24];
  RowSpec row = { .icon = -1 };
  int i = index->row;
  int rarity = game_codex_rarity(i);
  snprintf(sub, sizeof(sub), "No.%03d %s", i + 1, rarity == RARITY_NORMAL ? "" : g_rarity_names[rarity]);
  row.sub = sub;
  if (game_codex_seen(i)) {
    static char name[32];
    game_codex_name(i, name, sizeof(name));
    row.icon = game_codex_icon(i);
    row.title = name;
    row.tint_title = true;
    row.title_color = rarity_color(rarity);
  } else {
    row.title = "???";
    row.dim = true;
  }
  gfx_draw_row(ctx, cell, &row);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  gfx_items_acquire();
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
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

void codex_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
