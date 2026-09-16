#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 設定
//   ・自動帰還: HP が何%を切ったら帰るか（巻物があれば使い、なければ歩いて帰る）
//   ・危険の振動: 自動帰還や死亡のときに振動する（電池の消費が増える）
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return 2;
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[24];
  RowSpec row = { .icon = -1 };
  if (index->row == 0) {
    row.title = "Auto Return";
    int pct = game_auto_return_pct();
    if (pct) snprintf(sub, sizeof(sub), "Below %d%% HP", pct);
    else snprintf(sub, sizeof(sub), "Off (risky!)");
    row.warn_sub = pct == 0;
  } else {
    row.title = "Danger Vibe";
    snprintf(sub, sizeof(sub), game_vibrate() ? "On: more battery" : "Off");
  }
  row.sub = sub;
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->row == 0) {
    game_cycle_auto_return();
  } else {
    game_toggle_vibrate();
    if (game_vibrate()) vibes_short_pulse();
  }
  menu_layer_reload_data(menu);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
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
  window_destroy(window);
  s_window = NULL;
}

void settings_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
