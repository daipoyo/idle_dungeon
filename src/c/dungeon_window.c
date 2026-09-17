#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// ダンジョン選択（選ぶとそのまま出発）
//   前のダンジョンのボスを倒すと次が開く
//   町で貯めた歩数があれば見出しに出す（出発した瞬間に使われる）
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static GBitmap *s_enemy[6];
static GBitmap *s_enemy_sub[6];

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return DUNGEON_COUNT;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return game_rested_steps() > 0 ? MENU_HEADER_H : 0;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "RESTED %ld STEPS", (long)game_rested_steps());
  gfx_draw_header(ctx, cell, buf);
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
    snprintf(sub, sizeof(sub), "%dF %d steps", d->floors, d->floors * d->steps_per_floor);
  } else {
    snprintf(sub, sizeof(sub), "Beat %s", g_dungeons[i - 1].monsters[3]);
  }
  RowSpec row = {
    .icon = -1,
    .bitmap = unlocked ? s_enemy_sub[d->art] : NULL,
    .title = unlocked ? d->name : "???",
    .sub = sub,
    .right = game_dungeon_cleared(i) ? "CLEAR" : NULL,
    .tint_right = true,
    .right_color = THEME_GOLD,
    .dim = !unlocked,
    .warn_sub = !unlocked,
  };
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (!game_depart(index->row)) {
    vibes_short_pulse();
    return;
  }
  ui_state_changed();
  ui_back_to_scene();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  for (int i = 0; i < 6; i++) {
    s_enemy[i] = gbitmap_create_with_resource(gfx_enemy_resource(i));
    s_enemy_sub[i] = gbitmap_create_as_sub_bitmap(s_enemy[i], GRect(0, 0, ENEMY_SIZE, ENEMY_SIZE));
  }
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
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
  for (int i = 0; i < 6; i++) {
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
