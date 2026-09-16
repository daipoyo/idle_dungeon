#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 鍛冶屋
//   セクション0: 装備中の物、セクション1: 持ち物（鑑定済みの物だけ強化できる）
//   お金がかかり失敗もあるので、SELECT 2回（確認あり）で強化する。
//   カーソルを動かすと確認は取り消し。失敗してもお金が減るだけで、壊れない。
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static int s_confirm_section = -1;   // 確認待ちの行（-1 = なし）
static int s_confirm_row;
static const char *s_message;

// 装備枠のうち、何か着けている枠だけを並べる
static int equipped_slot(int row) {
  int n = 0;
  for (int s = 0; s < EQUIP_SLOTS; s++) {
    if (!game_equipped(s)) continue;
    if (n == row) return s;
    n++;
  }
  return -1;
}

static int equipped_count(void) {
  int n = 0;
  for (int s = 0; s < EQUIP_SLOTS; s++) n += game_equipped(s) ? 1 : 0;
  return n;
}

static const Item *item_at(int section, int row) {
  return section == 0 ? game_equipped(equipped_slot(row)) : game_bag(row);
}

static uint16_t get_num_sections(MenuLayer *menu, void *data) {
  return 2;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  int n = section == 0 ? equipped_count() : game_bag_count();
  return n > 0 ? n : 1;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  // 結果や確認の文言は一番上の見出しに出す
  if (section == 0 && s_message) {
    gfx_draw_header(ctx, cell, s_message);
    return;
  }
  if (section == 0 && s_confirm_section >= 0) {
    const Item *it = item_at(s_confirm_section, s_confirm_row);
    snprintf(buf, sizeof(buf), "SELECT AGAIN: %dG", game_upgrade_cost(it));
    gfx_draw_header(ctx, cell, buf);
    return;
  }
  if (section == 0) snprintf(buf, sizeof(buf), "SMITH %ldG", (long)game_gold());
  else snprintf(buf, sizeof(buf), "BAG");
  gfx_draw_header(ctx, cell, buf);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char name[40];
  static char sub[32];
  static char stats[24];
  RowSpec row = { .icon = -1 };
  const Item *it = item_at(index->section, index->row);
  if (!it) {
    row.title = "Nothing here";
    row.sub = index->section == 0 ? "Wear some gear" : "Bag is empty";
    row.dim = true;
    gfx_draw_row(ctx, cell, &row);
    return;
  }
  gfx_item_row(&row, it, name, sizeof(name), stats, sizeof(stats));
  if (!game_item_identified(it)) {
    row.sub = "Identify first";
    row.dim = true;
  } else if (it->plus >= MAX_PLUS) {
    row.sub = "Fully upgraded";
    row.warn_sub = false;
  } else {
    int cost = game_upgrade_cost(it);
    snprintf(sub, sizeof(sub), "+%d %dG %d%%", it->plus + 1, cost, game_upgrade_chance(it));
    row.sub = sub;
    row.warn_sub = game_gold() < cost;
  }
  gfx_draw_row(ctx, cell, &row);
}

static void clear_confirm(void) {
  s_confirm_section = -1;
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  const Item *it = item_at(index->section, index->row);
  if (!it) return;
  s_message = NULL;

  bool confirmed = s_confirm_section == index->section && s_confirm_row == index->row;
  if (!confirmed) {
    // 1回目: 強化できるかだけ確かめて、確認待ちにする
    if (!game_item_identified(it)) s_message = "IDENTIFY FIRST";
    else if (it->plus >= MAX_PLUS) s_message = "ALREADY +10";
    else if (game_gold() < game_upgrade_cost(it)) s_message = "NOT ENOUGH GOLD";
    if (s_message) {
      vibes_short_pulse();
    } else {
      s_confirm_section = index->section;
      s_confirm_row = index->row;
    }
    menu_layer_reload_data(menu);
    return;
  }

  clear_confirm();
  UpgradeResult r = index->section == 0 ? game_upgrade_equipped(equipped_slot(index->row))
                                        : game_upgrade_bag(index->row);
  switch (r) {
    case UPGRADE_OK: s_message = "SUCCESS!"; break;
    case UPGRADE_FAILED: s_message = "FAILED. GEAR IS SAFE"; vibes_short_pulse(); break;
    case UPGRADE_NO_GOLD: s_message = "NOT ENOUGH GOLD"; vibes_short_pulse(); break;
    case UPGRADE_MAX: s_message = "ALREADY +10"; break;
    case UPGRADE_NONE: break;
  }
  ui_state_changed();
  menu_layer_reload_data(menu);
}

static void selection_changed(MenuLayer *menu, MenuIndex new_index, MenuIndex old_index,
                              void *data) {
  // 別の行へ動いたら、確認も結果の文言も消す
  if (s_confirm_section >= 0 || s_message) {
    clear_confirm();
    s_message = NULL;
    menu_layer_reload_data(menu);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  gfx_items_acquire();
  clear_confirm();
  s_message = NULL;
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = get_num_sections,
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
    .selection_changed = selection_changed,
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

void smith_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
