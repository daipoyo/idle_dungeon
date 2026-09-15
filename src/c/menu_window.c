#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// メニュー（町にいるときとダンジョン探索中で項目が変わる）
// ============================================================
typedef enum {
  CMD_EXPLORE,
  CMD_DUNGEONS,
  CMD_SHOP,
  CMD_STATUS,
  CMD_RETREAT,
} Command;

static Window *s_window;
static MenuLayer *s_menu;

static int build_commands(Command *out) {
  int n = 0;
  if (game_location() == LOC_TOWN) {
    out[n++] = CMD_EXPLORE;
    out[n++] = CMD_DUNGEONS;
    out[n++] = CMD_SHOP;
    out[n++] = CMD_STATUS;
  } else {
    out[n++] = CMD_STATUS;
    out[n++] = CMD_RETREAT;
  }
  return n;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  Command cmds[5];
  return build_commands(cmds);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  Command cmds[5];
  int n = build_commands(cmds);
  if (index->row >= n) return;
  static char sub[32];
  RowSpec row = { .icon = -1 };
  switch (cmds[index->row]) {
    case CMD_EXPLORE:
      row.icon = 1;
      row.title = "Explore";
      snprintf(sub, sizeof(sub), "%s", g_dungeons[game_current_dungeon()].name);
      row.sub = sub;
      break;
    case CMD_DUNGEONS:
      row.icon = 14;
      row.title = "Dungeons";
      row.sub = "Pick a place";
      break;
    case CMD_SHOP:
      row.icon = 8;
      row.title = "Shop";
      row.sub = "Buy & sell";
      break;
    case CMD_STATUS:
      row.icon = 5;
      row.title = "Status";
      row.sub = game_can_change_gear() ? "Stats & gear" : "View stats";
      break;
    case CMD_RETREAT:
      row.icon = 12;
      row.title = "Retreat";
      row.sub = "Back, no loot";
      break;
  }
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  Command cmds[5];
  int n = build_commands(cmds);
  if (index->row >= n) return;
  switch (cmds[index->row]) {
    case CMD_EXPLORE:
      game_depart(game_current_dungeon());
      ui_back_to_scene();
      break;
    case CMD_DUNGEONS:
      dungeon_window_push();
      break;
    case CMD_SHOP:
      shop_window_push();
      break;
    case CMD_STATUS:
      status_window_push();
      break;
    case CMD_RETREAT:
      game_retreat();
      ui_back_to_scene();
      break;
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  gfx_items_acquire();
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
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

void menu_window_refresh(void) {
  if (s_menu) {
    menu_layer_reload_data(s_menu);
    menu_layer_set_selected_index(s_menu, MenuIndex(0, 0), MenuRowAlignCenter, false);
  }
}

void menu_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
