#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "npc_header.h"

// ============================================================
// 鍛冶屋
//   セクション0: 装備中の物、セクション1: 持ち物（鑑定済みの物だけ強化できる）
//   お金がかかり失敗もあるので、SELECT 2回（確認あり）で強化する。
//   カーソルを動かすと確認は取り消し。失敗してもお金が減るだけで、壊れない。
//   上にひげの職人の顔とセリフ。結果はセリフで伝える
// ============================================================
static Window *s_window;
static MenuLayer *s_menu;
static int s_confirm_section = -1;   // 確認待ちの行（-1 = なし）
static int s_confirm_row;
static Layer *s_header;
static const char *s_speech;

static const char *const GREETINGS[] = {
  "Need an edge? I'll sharpen it.",
  "Forge is hot. What'll it be?",
  "Bring me steel. I'll make it sing.",
};
static const char *const SUCCESS[] = {
  "Ha! Solid work, if I say so.",
  "Stronger than yesterday.",
  "That'll bite deeper now.",
};
static const char *const FAILED[] = {
  "Blast! It fought back. Gear's fine.",
  "Bah, no luck. Your gear is safe.",
};

static void say(const char *speech) {
  s_speech = speech;
  npc_header_say(s_header, speech);
}

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

// 強化で変わる性能（"ATK 74>80" のように、変わるものだけ）
static void upgrade_preview(const Item *it, char *buf, size_t size) {
  Item next = *it;
  next.plus++;
  ItemStats a = game_item_stats(it), b = game_item_stats(&next);
  static const char *const NAMES[3] = { "ATK", "DEF", "HP" };
  const int before[3] = { a.atk, a.def, a.hp }, after[3] = { b.atk, b.def, b.hp };
  int n = 0;
  buf[0] = '\0';
  for (int i = 0; i < 3 && n < (int)size; i++) {
    if (before[i] == after[i]) continue;
    n += snprintf(buf + n, size - n, "%s%s %d>%d", n ? " " : "", NAMES[i], before[i], after[i]);
  }
  if (!buf[0]) snprintf(buf, size, "No change");
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char name[40];
  static char sub[32];
  static char stats[24];
  RowSpec row = { .icon = -1, .enemy = -1 };
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
  } else if (s_confirm_section == index->section && s_confirm_row == index->row) {
    // 確認待ちの行: 強化するとどう変わるか
    upgrade_preview(it, sub, sizeof(sub));
    row.sub = sub;
    row.tint_sub = true;
    row.sub_color = THEME_HI;
  } else {
    int cost = game_upgrade_cost(it);
    snprintf(sub, sizeof(sub), "To +%d: %dG %d%%", it->plus + 1, cost, game_upgrade_chance(it));
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

  bool confirmed = s_confirm_section == index->section && s_confirm_row == index->row;
  if (!confirmed) {
    // 1回目: 強化できるかだけ確かめて、確認待ちにする
    const char *problem = NULL;
    if (!game_item_identified(it)) problem = "Can't work what I can't see.";
    else if (it->plus >= MAX_PLUS) problem = "Can't push it past +10.";
    else if (game_gold() < game_upgrade_cost(it)) problem = "No coin, no hammer.";
    if (problem) {
      say(problem);
      vibes_short_pulse();
    } else {
      s_confirm_section = index->section;
      s_confirm_row = index->row;
      say("That'll cost ya. Sure?");
    }
    menu_layer_reload_data(menu);
    return;
  }

  clear_confirm();
  UpgradeResult r = index->section == 0 ? game_upgrade_equipped(equipped_slot(index->row))
                                        : game_upgrade_bag(index->row);
  switch (r) {
    case UPGRADE_OK: say(npc_pick(NPC_LINES(SUCCESS), s_speech)); break;
    case UPGRADE_FAILED: say(npc_pick(NPC_LINES(FAILED), s_speech)); vibes_short_pulse(); break;
    case UPGRADE_NO_GOLD: say("No coin, no hammer."); vibes_short_pulse(); break;
    case UPGRADE_MAX: say("Can't push it past +10."); break;
    case UPGRADE_NONE: break;
  }
  ui_state_changed();
  menu_layer_reload_data(menu);
}

static void select_long_click(MenuLayer *menu, MenuIndex *index, void *data) {
  if (index->section == 0 && equipped_slot(index->row) >= 0) {
    item_window_push(ITEM_AT_EQUIP, equipped_slot(index->row), false);
  } else if (index->section == 1 && game_bag(index->row)) {
    item_window_push(ITEM_AT_BAG, index->row, false);
  }
}

static void selection_changed(MenuLayer *menu, MenuIndex new_index, MenuIndex old_index,
                              void *data) {
  // 別の行へ動いたら確認は取り消す
  if (s_confirm_section >= 0) {
    clear_confirm();
    menu_layer_reload_data(menu);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  clear_confirm();
  s_speech = npc_pick(NPC_LINES(GREETINGS), NULL);
  s_header = npc_header_create(GRect(0, 0, b.size.w, NPC_HEADER_H), RESOURCE_ID_IMG_FACE_SMITH, s_speech);
  layer_add_child(root, s_header);
  s_menu = menu_layer_create(GRect(0, NPC_HEADER_H, b.size.w, b.size.h - NPC_HEADER_H));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_sections = get_num_sections,
    .get_num_rows = get_num_rows,
    .get_header_height = get_header_height,
    .draw_header = draw_header,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
    .select_long_click = select_long_click,
    .selection_changed = selection_changed,
  });
  gfx_setup_menu(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  s_menu = NULL;
  npc_header_destroy(s_header);
  s_header = NULL;
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
