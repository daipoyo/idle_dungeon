#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// ステータス・装備画面
//   セクション0: ステータスカード（勇者の姿と能力値）
//   セクション1: 装備中（武器・防具・装飾品）… 選択で外す
//   セクション2: 持ち物 … 選択で装備／解除
// ============================================================
#define CARD_H (IS_LARGE_SCREEN ? 88 : 80)

static Window *s_window;
static MenuLayer *s_menu;

static const char *const SLOT_NAMES[3] = { "Weapon", "Armor", "Acc." };

static void format_stats(char *buf, size_t size, int id) {
  const ItemDef *it = &g_items[id];
  if (it->type == ITEM_MATERIAL) {
    snprintf(buf, size, "Sell %dG", game_sell_price(id));
  } else if (it->atk && it->def) {
    snprintf(buf, size, "A+%d D+%d", it->atk, it->def);
  } else if (it->atk) {
    snprintf(buf, size, "ATK+%d", it->atk);
  } else {
    snprintf(buf, size, "DEF+%d", it->def);
  }
}

static void draw_card(GContext *ctx, const Layer *cell) {
  GRect b = layer_get_bounds(cell);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  // 勇者の立ち絵（枠の中に大きめに描く）
  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 8, 2));
  int scale = 3;
  int slot_w = 20 * scale, slot_h = CARD_H - 4 * PX;
  GRect slot = GRect(inset, 2 * PX, slot_w, slot_h);
  gfx_draw_window(ctx, slot);
  graphics_context_set_fill_color(ctx, GColorDarkGreen);
  graphics_fill_rect(ctx, GRect(slot.origin.x + 2 * PX, slot.origin.y + slot_h - 7 * PX,
                                slot_w - 4 * PX, 5 * PX), 0, GCornerNone);
  int hero_y = slot.origin.y + slot_h - 5 * PX - (HERO_MAP_H - 1) * scale;
  gfx_draw_hero(ctx, slot.origin.x + 3 * scale, hero_y, HERO_POSE_IDLE, scale, false);

  int x = slot.origin.x + slot_w + 3 * PX;
  int w = b.size.w - x - inset;
  int y = slot.origin.y + 2 * PX;
  static char buf[32];
  snprintf(buf, sizeof(buf), "POWER %d", game_power());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_GOLD);
  y += LINE_H + PX;
  snprintf(buf, sizeof(buf), "ATK %d", BASE_POWER + game_atk());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_FG);
  y += LINE_H;
  snprintf(buf, sizeof(buf), "DEF %d", game_def());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_FG);
  y += LINE_H;
  snprintf(buf, sizeof(buf), "%ld", (long)game_gold());
  gfx_draw_coin(ctx, x, y);
  gfx_text(ctx, buf, GRect(x + 6 * PX, y, w - 6 * PX, LINE_H), GTextAlignmentLeft, THEME_GOLD);
  y += LINE_H;
  if (game_can_change_gear()) {
    snprintf(buf, sizeof(buf), "WIN %d/%d", game_wins(), game_runs());
    gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_SUB);
  } else {
    gfx_text(ctx, "EXPLORING", GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_WARN);
  }
}

// ------------------------------------------------------------
// MenuLayer コールバック
// ------------------------------------------------------------
static uint16_t get_num_sections(MenuLayer *menu, void *data) {
  return 3;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  if (section == 0) return 1;
  if (section == 1) return 3;
  int n = game_owned_kinds();
  return n > 0 ? n : 1;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return section == 0 ? 0 : MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  if (section == 1) {
    gfx_draw_header(ctx, cell, game_can_change_gear() ? "EQUIPMENT" : "EQUIP (LOCKED)");
  } else if (section == 2) {
    gfx_draw_header(ctx, cell, "BAG");
  }
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return index->section == 0 ? CARD_H : ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[32];
  static char right[8];
  if (index->section == 0) {
    draw_card(ctx, cell);
    return;
  }
  RowSpec row = { .icon = -1 };
  if (index->section == 1) {
    int id = game_equipped((ItemType)index->row);
    if (id >= 0) {
      row.icon = id;
      row.title = g_items[id].name;
      char stats[24];
      format_stats(stats, sizeof(stats), id);
      snprintf(sub, sizeof(sub), "%s %s", SLOT_NAMES[index->row], stats);
    } else {
      row.title = "(none)";
      snprintf(sub, sizeof(sub), "%s", SLOT_NAMES[index->row]);
      row.dim = true;
    }
    row.sub = sub;
  } else {
    int id = game_owned_nth(index->row);
    if (id < 0) {
      row.title = "Empty";
      row.sub = "Go find loot!";
      row.dim = true;
    } else {
      row.icon = id;
      row.title = g_items[id].name;
      format_stats(sub, sizeof(sub), id);
      row.sub = sub;
      snprintf(right, sizeof(right), "%sx%d", game_is_equipped(id) ? "E " : "", game_item_count(id));
      row.right = right;
    }
  }
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == 0) return;
  if (!game_can_change_gear()) {
    vibes_short_pulse();
    return;
  }
  if (index->section == 1) {
    game_unequip((ItemType)index->row);
  } else {
    int id = game_owned_nth(index->row);
    if (id < 0 || g_items[id].type == ITEM_MATERIAL) return;
    game_toggle_equip(id);
  }
  menu_layer_reload_data(s_menu);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  gfx_items_acquire();
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = get_num_sections,
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
  gfx_items_release();
  window_destroy(window);
  s_window = NULL;
}

void status_window_refresh(void) {
  if (s_menu) menu_layer_reload_data(s_menu);
}

void status_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
