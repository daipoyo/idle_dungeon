#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// ダンジョン選択（選ぶとそのまま出発）
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static GBitmap *s_enemy[DUNGEON_COUNT];
static GBitmap *s_enemy_sub[DUNGEON_COUNT];

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return DUNGEON_COUNT;
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H + 4;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  int i = index->row;
  const DungeonDef *d = &g_dungeons[i];
  bool unlocked = game_dungeon_unlocked(i);
  static char sub[32];
  if (unlocked) {
    snprintf(sub, sizeof(sub), "Power %d / %ds", d->required_power, d->clear_time_sec);
  } else {
    snprintf(sub, sizeof(sub), "LOCKED: need %d", d->required_power);
  }
  RowSpec row = {
    .icon = -1,
    .bitmap = s_enemy_sub[i],
    .title = d->name,
    .sub = sub,
    .right = (i == game_current_dungeon()) ? "*" : NULL,
    .dim = !unlocked,
    .warn_sub = !unlocked,
  };
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (!game_dungeon_unlocked(index->row)) {
    vibes_short_pulse();
    return;
  }
  game_depart(index->row);
  ui_back_to_scene();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  for (int i = 0; i < DUNGEON_COUNT; i++) {
    s_enemy[i] = gbitmap_create_with_resource(gfx_enemy_resource(i));
    s_enemy_sub[i] = gbitmap_create_as_sub_bitmap(s_enemy[i], GRect(0, 0, 24, 24));
  }
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
  });
  gfx_setup_menu(s_menu, window);
  menu_layer_set_selected_index(s_menu, MenuIndex(0, game_current_dungeon()), MenuRowAlignCenter, false);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  s_menu = NULL;
  for (int i = 0; i < DUNGEON_COUNT; i++) {
    gbitmap_destroy(s_enemy_sub[i]);
    gbitmap_destroy(s_enemy[i]);
    s_enemy_sub[i] = NULL;
    s_enemy[i] = NULL;
  }
  window_destroy(window);
  s_window = NULL;
}

void dungeon_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
