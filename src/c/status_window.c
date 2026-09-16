#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// ステータス・装備画面
//   セクション0: ステータスカード（勇者の姿と能力値）
//   セクション1: 装備（9か所）… 選ぶと外して持ち物へ
//   セクション2: 持ち物 … 選ぶと装備する（町にいるときだけ）
// ============================================================
#define CARD_H (IS_LARGE_SCREEN ? 88 : 80)

static Window *s_window;
static MenuLayer *s_menu;

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
  snprintf(buf, sizeof(buf), "LV %d", game_level());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_GOLD);
  y += LINE_H + PX;
  snprintf(buf, sizeof(buf), "HP %d/%d", game_hp(), game_max_hp());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_FG);
  y += LINE_H;
  snprintf(buf, sizeof(buf), "ATK %d", game_atk());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_FG);
  y += LINE_H;
  snprintf(buf, sizeof(buf), "DEF %d", game_def());
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_FG);
  y += LINE_H;
  if (game_level() < MAX_LEVEL) {
    snprintf(buf, sizeof(buf), "XP %ld/%ld", (long)game_xp(), (long)game_xp_next());
  } else {
    snprintf(buf, sizeof(buf), "XP MAX");
  }
  gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, THEME_SUB);
}

// ------------------------------------------------------------
// MenuLayer コールバック
// ------------------------------------------------------------
static uint16_t get_num_sections(MenuLayer *menu, void *data) {
  return 3;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  if (section == 0) return 1;
  if (section == 1) return EQUIP_SLOTS;
  int n = game_bag_count();
  return n > 0 ? n : 1;
}

static int16_t get_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return section == 0 ? 0 : MENU_HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[24];
  if (section == 1) {
    gfx_draw_header(ctx, cell, game_can_change_gear() ? "EQUIPMENT" : "EQUIP (LOCKED)");
  } else if (section == 2) {
    snprintf(buf, sizeof(buf), "BAG %d/%d", game_bag_count(), BAG_SIZE);
    gfx_draw_header(ctx, cell, buf);
  }
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return index->section == 0 ? CARD_H : ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[32];
  if (index->section == 0) {
    draw_card(ctx, cell);
    return;
  }
  RowSpec row = { .icon = -1 };
  const Item *it;
  if (index->section == 1) {
    it = game_equipped(index->row);
    if (!it) {
      row.title = "(none)";
      row.sub = g_slot_names[index->row];
      row.dim = true;
      gfx_draw_row(ctx, cell, &row);
      return;
    }
  } else {
    it = game_bag(index->row);
    if (!it) {
      row.title = "Empty";
      row.sub = "Go find loot!";
      row.dim = true;
      gfx_draw_row(ctx, cell, &row);
      return;
    }
  }
  const BaseDef *b = game_item_base(it);
  row.icon = b->icon;
  row.title = b->name;
  gfx_item_stat_text(it, sub, sizeof(sub));
  row.sub = sub;
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == 0) return;
  bool ok;
  if (index->section == 1) {
    ok = game_unequip(index->row);
  } else {
    ok = game_equip_from_bag(index->row);
  }
  if (!ok) {
    vibes_short_pulse();
    return;
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
