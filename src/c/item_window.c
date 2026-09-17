#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// アイテムの詳細
//   一覧に入りきらない長い名前・接辞・特殊効果・セットの進み具合を見せる。
//   持ち物画面から開いたときは SELECT で装備・取り外し・巻物での鑑定ができる。
//   UP / DOWN でスクロール。
// ============================================================
static Window *s_window;
static Layer *s_frame;
static Layer *s_content;
static ItemPlace s_place;
static int s_index;
static bool s_allow_action;
static Item s_copy;          // お店の品など、持ち物にない物
static int s_scroll;
static int s_content_h;
static const char *s_message;

typedef enum { ACT_NONE, ACT_EQUIP, ACT_UNEQUIP, ACT_SCROLL } Action;

static const Item *current_item(void) {
  switch (s_place) {
    case ITEM_AT_EQUIP: return game_equipped(s_index);
    case ITEM_AT_BAG: return game_bag(s_index);
    case ITEM_AT_STASH: return game_stash(s_index);
    default: return s_copy.base ? &s_copy : NULL;
  }
}

// SELECT でできること（hint に画面下の案内を入れる）
static Action current_action(const Item *it, const char **hint) {
  *hint = NULL;
  if (!s_allow_action || !it) return ACT_NONE;
  if (s_place == ITEM_AT_EQUIP) {
    if (!game_can_change_gear()) {
      *hint = "Gear is locked in the dungeon";
      return ACT_NONE;
    }
    *hint = "SELECT: Take off";
    return ACT_UNEQUIP;
  }
  if (s_place != ITEM_AT_BAG) return ACT_NONE;
  if (!game_item_identified(it)) {
    if (game_identify_scrolls() > 0) {
      *hint = "SELECT: Read identify scroll";
      return ACT_SCROLL;
    }
    *hint = "Take it to the Appraiser";
    return ACT_NONE;
  }
  if (!game_can_change_gear()) {
    *hint = "Gear is locked in the dungeon";
    return ACT_NONE;
  }
  *hint = "SELECT: Equip";
  return ACT_EQUIP;
}

// 上から順に文字を置いていく。枠の外は content レイヤーが切り取る
typedef struct {
  GContext *ctx;
  int width;
  int y;
} Pen;

static void pen_text(Pen *p, const char *text, GColor color) {
  if (!text || !text[0]) return;
  int h = gfx_text_lines(text, p->width) * LINE_H;
  gfx_text(p->ctx, text, GRect(0, p->y - s_scroll, p->width, h), GTextAlignmentLeft, color);
  p->y += h;
}

static void pen_gap(Pen *p) {
  p->y += 2 * PX;
}

static void draw_content(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  const Item *it = current_item();
  if (!it) return;
  static char buf[48];
  static char stat[32];
  Pen p = { ctx, b.size.w, 0 };

  // アイコンと名前（名前は長ければ折り返す）
  int name_x = ITEM_ICON_SIZE + 2 * PX;
  game_item_name(it, buf, sizeof(buf));
  int name_h = gfx_text_lines(buf, b.size.w - name_x) * LINE_H;
  gfx_draw_item(ctx, it, 0, -s_scroll);
  gfx_draw_item_glow(ctx, it, 0, -s_scroll, 0);
  gfx_text(ctx, buf, GRect(name_x, -s_scroll, b.size.w - name_x, name_h), GTextAlignmentLeft,
           gfx_rarity_color(it));
  p.y = (name_h > ITEM_ICON_SIZE ? name_h : ITEM_ICON_SIZE) + PX;

  bool identified = game_item_identified(it);
  const ShapeDef *sh = game_item_shape(it);
  snprintf(buf, sizeof(buf), "%s %s  Lv %d", g_rarity_names[it->rarity % RARITY_COUNT],
           sh ? g_slot_names[sh->slot] : "", it->ilvl);
  pen_text(&p, buf, THEME_SUB);
  pen_gap(&p);

  if (!identified) {
    pen_text(&p, "Unidentified", THEME_WARN);
    pen_text(&p, "Its power is hidden", THEME_SUB);
  } else {
    gfx_item_stat_text(it, stat, sizeof(stat));
    pen_text(&p, stat, THEME_FG);
    if (it->plus) {
      snprintf(buf, sizeof(buf), "Upgraded +%d", it->plus);
      pen_text(&p, buf, THEME_FG);
    }
  }

  // 接辞
  for (int i = 0; i < game_item_affix_count(it); i++) {
    int value = 0;
    const AffixDef *a = game_item_affix(it, i, &value);
    if (!a) continue;
    static const char *const STAT[] = { "ATK", "DEF", "HP" };
    snprintf(buf, sizeof(buf), "%s%s %s+%d", i % 2 ? "of " : "", a->name, STAT[a->stat % 3], value);
    pen_text(&p, buf, GColorPictonBlue);
  }

  // 固有・セット装備の特殊効果
  int special = game_item_special(it);
  if (special >= 0 && identified) {
    const SpecialDef *d = &g_specials[special];
    pen_gap(&p);
    game_effect_text((Effect)d->fx1, d->val1, buf, sizeof(buf));
    pen_text(&p, buf, gfx_rarity_color(it));
    game_effect_text((Effect)d->fx2, d->val2, buf, sizeof(buf));
    pen_text(&p, buf, gfx_rarity_color(it));

    const SetDef *set = game_item_set(it);
    if (set) {
      int worn = game_set_pieces_equipped(it);
      char set_name[ITEM_NAME_LEN];
      item_set_name((int)(set - g_sets), set_name);
      snprintf(buf, sizeof(buf), "%s set %d/3", set_name, worn);
      pen_text(&p, buf, GColorBrightGreen);
      game_effect_text((Effect)set->fx2, set->val2, stat, sizeof(stat));
      snprintf(buf, sizeof(buf), "2: %s", stat);
      pen_text(&p, buf, worn >= 2 ? GColorBrightGreen : THEME_DIM);
      game_effect_text((Effect)set->fx3, set->val3, stat, sizeof(stat));
      snprintf(buf, sizeof(buf), "3: %s", stat);
      pen_text(&p, buf, worn >= 3 ? GColorBrightGreen : THEME_DIM);
    }
  }

  pen_gap(&p);
  if (s_place != ITEM_AT_SHOP) {
    snprintf(buf, sizeof(buf), "Sells for %dG", game_item_price(it));
    pen_text(&p, buf, THEME_SUB);
  }

  const char *hint;
  current_action(it, &hint);
  if (s_message) hint = s_message;
  if (hint) {
    pen_gap(&p);
    pen_text(&p, hint, s_message ? THEME_WARN : THEME_HI);
  }
  s_content_h = p.y;
}

static void draw_frame(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 7, 0));
  GRect panel = GRect(inset, PBL_IF_ROUND_ELSE(SNAP(b.size.h / 8), 0), b.size.w - inset * 2,
                      b.size.h - PBL_IF_ROUND_ELSE(SNAP(b.size.h / 4), 0));
  gfx_draw_window(ctx, panel);
}

static void scroll_by(int dy) {
  GRect view = layer_get_bounds(s_content);
  int max = s_content_h - view.size.h;
  s_scroll += dy;
  if (s_scroll > max) s_scroll = max;
  if (s_scroll < 0) s_scroll = 0;
  layer_mark_dirty(s_content);
}

static void up_click(ClickRecognizerRef rec, void *ctx) { scroll_by(-3 * LINE_H); }
static void down_click(ClickRecognizerRef rec, void *ctx) { scroll_by(3 * LINE_H); }

static void select_click(ClickRecognizerRef rec, void *ctx) {
  const Item *it = current_item();
  const char *hint;
  Action action = current_action(it, &hint);
  bool ok = false;
  s_message = NULL;
  switch (action) {
    case ACT_EQUIP: ok = game_equip_from_bag(s_index); break;
    case ACT_UNEQUIP:
      ok = game_unequip(s_index);
      if (!ok) s_message = "Bag is full";
      break;
    case ACT_SCROLL: ok = game_identify_with_scroll(s_index) == IDENT_OK; break;
    case ACT_NONE: return;
  }
  if (!ok) {
    vibes_short_pulse();
    layer_mark_dirty(s_content);
    return;
  }
  ui_state_changed();
  if (action == ACT_SCROLL) {
    // 鑑定したらその場で正体を見せる
    s_scroll = 0;
    layer_mark_dirty(s_content);
    return;
  }
  window_stack_pop(true);
}

static void click_config(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 150, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  gfx_items_acquire();
  s_frame = layer_create(b);
  layer_set_update_proc(s_frame, draw_frame);
  layer_add_child(root, s_frame);

  int inset = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 7, 0));
  GRect panel = GRect(inset, PBL_IF_ROUND_ELSE(SNAP(b.size.h / 8), 0), b.size.w - inset * 2,
                      b.size.h - PBL_IF_ROUND_ELSE(SNAP(b.size.h / 4), 0));
  s_content = layer_create(gfx_window_inner(panel));
  layer_set_update_proc(s_content, draw_content);
  layer_add_child(root, s_content);
}

static void window_unload(Window *window) {
  layer_destroy(s_content);
  layer_destroy(s_frame);
  s_content = s_frame = NULL;
  gfx_items_release();
  window_destroy(window);
  s_window = NULL;
}

static void push(void) {
  if (s_window) return;
  s_scroll = 0;
  s_content_h = 0;
  s_message = NULL;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

void item_window_push(ItemPlace place, int index, bool allow_action) {
  s_place = place;
  s_index = index;
  s_allow_action = allow_action;
  memset(&s_copy, 0, sizeof(s_copy));
  push();
}

void item_window_push_copy(const Item *it) {
  s_place = ITEM_AT_SHOP;
  s_index = -1;
  s_allow_action = false;
  s_copy = *it;
  push();
}
