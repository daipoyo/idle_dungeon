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
#define GLOW_MS 450   // きらめきのコマ送り（この画面を開いている間だけ）

static Window *s_window;
static MenuLayer *s_menu;
static AppTimer *s_glow_timer;
static int s_glow_frame;

// 装備中の固有装備の数と、一番そろっているセットの部位数
static void equipped_specials(int *uniques, int *best_set_pieces) {
  *uniques = 0;
  *best_set_pieces = 0;
  for (int slot = 0; slot < EQUIP_SLOTS; slot++) {
    const Item *it = game_equipped(slot);
    if (!gfx_item_has_glow(it)) continue;
    if (it->rarity == RARITY_UNIQUE) (*uniques)++;
    int pieces = game_set_pieces_equipped(it);
    if (pieces > *best_set_pieces) *best_set_pieces = pieces;
  }
}

// セットボーナスか固有装備があるときは、ステータス欄に1行足す
static bool card_has_badge(void) {
  int uniques, pieces;
  equipped_specials(&uniques, &pieces);
  return uniques > 0 || pieces >= 2;
}

static int card_height(void) {
  return CARD_H + (card_has_badge() ? LINE_H : 0);
}

static void draw_card(GContext *ctx, const Layer *cell) {
  GRect b = layer_get_bounds(cell);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  int uniques, set_pieces;
  equipped_specials(&uniques, &set_pieces);
  bool set_bonus = set_pieces >= 2;
  // 固有装備なら金、セットボーナスなら緑の枠
  GColor aura = uniques > 0 ? GColorChromeYellow : (set_bonus ? GColorBrightGreen : GColorWhite);

  // 勇者の立ち絵（枠の中に大きめに描く）
  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 8, 2));
  int scale = 3;
  int slot_w = 20 * scale, slot_h = card_height() - 4 * PX;
  GRect slot = GRect(inset, 2 * PX, slot_w, slot_h);
  gfx_draw_window_color(ctx, slot, aura);
  graphics_context_set_fill_color(ctx, GColorDarkGreen);
  graphics_fill_rect(ctx, GRect(slot.origin.x + 2 * PX, slot.origin.y + slot_h - 7 * PX,
                                slot_w - 4 * PX, 5 * PX), 0, GCornerNone);
  int hero_y = slot.origin.y + slot_h - 5 * PX - (HERO_MAP_H - 1) * scale;
  gfx_draw_hero(ctx, slot.origin.x + 3 * scale, hero_y, HERO_POSE_IDLE, scale, false);
  if (uniques > 0 || set_bonus) {
    // 勇者のまわりの3か所で、少しずつずれて瞬く
    static const int8_t SPOT[3][3] = { { 6, 6, 0 }, { 24, 11, 1 }, { 23, 27, 3 } };
    for (int i = 0; i < 3; i++) {
      gfx_draw_twinkle(ctx, slot.origin.x + SPOT[i][0] * PX, slot.origin.y + SPOT[i][1] * PX,
                       s_glow_frame + SPOT[i][2], aura);
    }
  }

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

  if (set_bonus) {
    y += LINE_H;
    snprintf(buf, sizeof(buf), "SET BONUS %d/3", set_pieces);
    gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, GColorBrightGreen);
  } else if (uniques > 0) {
    y += LINE_H;
    snprintf(buf, sizeof(buf), "UNIQUE x%d", uniques);
    gfx_text(ctx, buf, GRect(x, y, w, LINE_H), GTextAlignmentLeft, GColorChromeYellow);
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
    gfx_draw_header(ctx, cell, "EQUIPMENT");
  } else if (section == 2) {
    snprintf(buf, sizeof(buf), "BAG %d/%d", game_bag_count(), BAG_SIZE);
    gfx_draw_header(ctx, cell, buf);
  }
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return index->section == 0 ? card_height() : ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[32];
  if (index->section == 0) {
    draw_card(ctx, cell);
    return;
  }
  RowSpec row = { .icon = -1, .enemy = -1 };
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
  static char name[40];
  static char pieces[6];
  gfx_item_row(&row, it, name, sizeof(name), sub, sizeof(sub));
  // 装備中のセット装備は、そろった部位数を緑で出す（強化値があるときはそちらを優先）
  if (index->section == 1 && gfx_item_has_glow(it) && game_item_set(it) && !it->plus) {
    snprintf(pieces, sizeof(pieces), "%d/3", game_set_pieces_equipped(it));
    row.right = pieces;
    row.tint_right = true;
    row.right_color = GColorBrightGreen;
  }
  if (!game_item_identified(it)) {
    // 未鑑定は巻物で鑑定できる。持っていなければ町の鑑定屋へ
    snprintf(sub, sizeof(sub), "Unidentified");
    row.warn_sub = true;
  }
  // 持ち物は、今つけている物より良くなるかどうかを右端の三角で示す
  if (index->section == 2) row.compare = game_item_compare(it, NULL, NULL);
  gfx_draw_row(ctx, cell, &row);
}

// 選ぶと詳細を開く。装備・取り外し・巻物での鑑定は詳細画面で行う
static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == 1 && game_equipped(index->row)) {
    item_window_push(ITEM_AT_EQUIP, index->row, true);
  } else if (index->section == 2 && game_bag(index->row)) {
    item_window_push(ITEM_AT_BAG, index->row, true);
  }
}

static bool anything_glows(void) {
  for (int i = 0; i < EQUIP_SLOTS; i++) if (gfx_item_has_glow(game_equipped(i))) return true;
  for (int i = 0; i < BAG_SIZE; i++) if (gfx_item_has_glow(game_bag(i))) return true;
  return false;
}

static void glow_tick(void *data) {
  s_glow_timer = NULL;
  if (!s_menu) return;
  s_glow_frame++;
  gfx_set_glow_frame(s_glow_frame);
  // 飾る物があるときだけ描き直す（なければ待つだけ）
  if (anything_glows()) layer_mark_dirty(menu_layer_get_layer(s_menu));
  s_glow_timer = app_timer_register(GLOW_MS, glow_tick, NULL);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
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
  s_glow_frame = 0;
  gfx_set_glow_frame(0);
  s_glow_timer = app_timer_register(GLOW_MS, glow_tick, NULL);
}

static void window_unload(Window *window) {
  if (s_glow_timer) app_timer_cancel(s_glow_timer);
  s_glow_timer = NULL;
  gfx_set_glow_frame(0);
  menu_layer_destroy(s_menu);
  s_menu = NULL;
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
