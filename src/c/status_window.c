#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// ステータス・装備画面
//   セクション0: ステータスカード（勇者の姿と能力値）
//   セクション1: 装備中（武器・防具・装飾品）… 選択で外す
//   セクション2: 持ち物 … 選択で装備／解除
// ============================================================
#define CARD_H (IS_LARGE_SCREEN ? 100 : 78)
#define HEADER_H 18

static Window *s_window;
static MenuLayer *s_menu;

static const char *const SLOT_NAMES[3] = { "Weapon", "Armor", "Accessory" };

static void format_stats(char *buf, size_t size, int id) {
  const ItemDef *it = &g_items[id];
  if (it->type == ITEM_MATERIAL) {
    snprintf(buf, size, "Material (sell %dG)", game_sell_price(id));
  } else if (it->atk && it->def) {
    snprintf(buf, size, "ATK+%d DEF+%d", it->atk, it->def);
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

  int inset = PBL_IF_ROUND_ELSE(b.size.w / 8, 4);
  int scale = 2;
  int slot_w = 52, slot_h = CARD_H - 10;
  GRect slot = GRect(inset, 5, slot_w, slot_h);
  graphics_context_set_fill_color(ctx, GColorDukeBlue);
  graphics_fill_rect(ctx, slot, 4, GCornersAll);
  graphics_context_set_fill_color(ctx, GColorArmyGreen);
  graphics_fill_rect(ctx, GRect(slot.origin.x, slot.origin.y + slot_h - 8, slot_w, 8), 4, GCornersBottom);
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_draw_round_rect(ctx, slot, 4);
  int hero_y = slot.origin.y + slot_h - 6 - HERO_H * scale;
  gfx_draw_hero(ctx, slot.origin.x + 2, hero_y, HERO_POSE_IDLE, scale, false);

  int x = slot.origin.x + slot_w + 6;
  int w = b.size.w - x - inset;
  int line = IS_LARGE_SCREEN ? 21 : 16;
  int y = 1;
  static char buf[32];
  snprintf(buf, sizeof(buf), "POWER %d", game_power());
  gfx_draw_text(ctx, buf, gfx_font_title(), GRect(x, y, w, line + 8), GTextAlignmentLeft, GColorChromeYellow);
  y += line + 4;
  snprintf(buf, sizeof(buf), "ATK %d  DEF %d", BASE_POWER + game_atk(), game_def());
  gfx_draw_text(ctx, buf, gfx_font_small_bold(), GRect(x, y, w, line + 4), GTextAlignmentLeft, GColorWhite);
  y += line;
  snprintf(buf, sizeof(buf), "%ld", (long)game_gold());
  gfx_draw_coin(ctx, x + 4, y + line / 2 + 2);
  gfx_draw_text(ctx, buf, gfx_font_small_bold(), GRect(x + 11, y, w - 11, line + 4), GTextAlignmentLeft, GColorIcterine);
  y += line;
  if (game_can_change_gear()) {
    snprintf(buf, sizeof(buf), "Wins %d / %d", game_wins(), game_runs());
    gfx_draw_text(ctx, buf, gfx_font_small(), GRect(x, y, w, line + 4), GTextAlignmentLeft, GColorLightGray);
  } else {
    gfx_draw_text(ctx, "Exploring...", gfx_font_small(), GRect(x, y, w, line + 4), GTextAlignmentLeft, GColorMelon);
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
  return section == 0 ? 0 : HEADER_H;
}

static void draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  if (section == 1) {
    gfx_draw_header(ctx, cell, game_can_change_gear() ? "EQUIPMENT" : "EQUIPMENT (locked)");
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
      snprintf(sub, sizeof(sub), "%s  %s", SLOT_NAMES[index->row], stats);
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
      row.sub = "Explore to find loot!";
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
