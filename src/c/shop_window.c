#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "item_art.h"
#include <stdlib.h>

// ============================================================
// お店
//   店頭画面 : 店内の背景・カウンター越しの女主人・会話ウィンドウ
//              上下ボタンで Buy / Sell / Talk を選び、SELECTで決定
//   購入・売却: 女主人の顔とセリフ付きのアイテム一覧
// ============================================================

typedef enum { OPT_BUY, OPT_SELL, OPT_TALK, OPT_COUNT } ShopOption;
static const char *const OPTION_LABELS[OPT_COUNT] = { "Buy", "Sell", "Talk" };

// ドットフォントは1行に入る文字数が少ないので、セリフは短めにしてある
static const char *const GREETINGS[] = {
  "Welcome, darling~",
  "Oh, it's you again~",
  "Hi handsome~ Need something?",
};
static const char *const TALKS[] = {
  "You look strong today~",
  "Come back safe, okay?",
  "I saved the best for you~",
  "Don't stare too long~",
  "Bring me something shiny?",
  "Brave men are my type~",
};
static const char *const THANKS[] = {
  "Great choice, darling~",
  "It suits you~",
  "A man of taste~",
};

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *s_speech;

// ---- 店頭 ----
static Window *s_front_window;
static Layer *s_front_layer;
static GBitmap *s_scene;          // 店内の一枚絵（画面と同じ大きさ）
static ShopOption s_option;

// ---- 一覧（購入・売却で共用） ----
typedef enum { LIST_BUY, LIST_SELL } ListMode;
static Window *s_list_window;
static Layer *s_list_header;
static MenuLayer *s_list_menu;
static GBitmap *s_face;
static ListMode s_mode;

static const char *pick(const char *const *lines, int n) {
  return lines[rand() % n];
}

// ============================================================
// 店頭画面
//   店内の絵は画面と同じ大きさの一枚絵（機種ごとに別ファイル）。
//   その上に会話ウィンドウを重ねる。
// ============================================================
static void front_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int w = b.size.w, h = b.size.h;
  // 会話ウィンドウの高さ（tools/gb_scenes.py の SHOP_SCREENS と揃える）
#if defined(PBL_ROUND)
  int dlg_h = h * 34 / 100;
#else
  int dlg_h = IS_LARGE_SCREEN ? 72 : 56;
#endif

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  if (s_scene) {
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, s_scene, b);
  }

  // 会話ウィンドウ
  int top = SNAP(h - dlg_h);
#if defined(PBL_ROUND)
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, top, w, h - top), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, top + PX, w, PX), 0, GCornerNone);
  int inset = SNAP(w / 7);
  GRect inner = GRect(inset, top + 3 * PX, w - inset * 2, h - top - 3 * PX);
  GTextAlignment align = GTextAlignmentCenter;
#else
  GRect panel = GRect(0, top, w, h - top);
  gfx_draw_window(ctx, panel);
  GRect inner = gfx_window_inner(panel);
  GTextAlignment align = GTextAlignmentLeft;
  gfx_draw_heart(ctx, inner.origin.x + inner.size.w - 3 * PX, inner.origin.y);
#endif

  GRect speech = GRect(inner.origin.x, inner.origin.y, inner.size.w - PBL_IF_ROUND_ELSE(0, 5 * PX),
                       LINE_H * 2);
  gfx_text(ctx, s_speech, speech, align, THEME_FG);

  // 選択肢（カーソルで選ぶ）
  // 丸型は下へ行くほど狭いので、セリフのすぐ下に置く
  int opt_y = PBL_IF_ROUND_ELSE(inner.origin.y + LINE_H * 2 + PX,
                                inner.origin.y + inner.size.h - LINE_H);
  int opt_area = PBL_IF_ROUND_ELSE(w * 7 / 10, inner.size.w);
  int opt_x = PBL_IF_ROUND_ELSE((w - opt_area) / 2, inner.origin.x);
  int opt_w = opt_area / OPT_COUNT;
  for (int i = 0; i < OPT_COUNT; i++) {
    int x = SNAP(opt_x + i * opt_w);
    bool sel = (i == (int)s_option);
    if (sel) gfx_draw_cursor(ctx, x, opt_y, THEME_HI);
    gfx_text(ctx, OPTION_LABELS[i], GRect(x + 5 * PX, opt_y, opt_w, LINE_H), GTextAlignmentLeft,
             sel ? THEME_HI : THEME_FG);
  }
}

static void list_window_push(ListMode mode);

static void front_select(ClickRecognizerRef rec, void *ctx) {
  switch (s_option) {
    case OPT_BUY:
      list_window_push(LIST_BUY);
      break;
    case OPT_SELL:
      list_window_push(LIST_SELL);
      break;
    default: {
      const char *prev = s_speech;
      while (s_speech == prev) s_speech = pick(TALKS, ARRAY_LEN(TALKS));
      layer_mark_dirty(s_front_layer);
      break;
    }
  }
}

static void front_up(ClickRecognizerRef rec, void *ctx) {
  s_option = (ShopOption)((s_option + OPT_COUNT - 1) % OPT_COUNT);
  layer_mark_dirty(s_front_layer);
}

static void front_down(ClickRecognizerRef rec, void *ctx) {
  s_option = (ShopOption)((s_option + 1) % OPT_COUNT);
  layer_mark_dirty(s_front_layer);
}

static void front_click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, front_select);
  window_single_click_subscribe(BUTTON_ID_UP, front_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, front_down);
}

static void front_load_art(void) {
  if (!s_scene) s_scene = gbitmap_create_with_resource(RESOURCE_ID_IMG_SHOP_SCENE);
}

static void front_unload_art(void) {
  if (s_scene) {
    gbitmap_destroy(s_scene);
    s_scene = NULL;
  }
}

static void front_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_front_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_front_layer, front_update_proc);
  layer_add_child(root, s_front_layer);
  s_option = OPT_BUY;
  s_speech = pick(GREETINGS, ARRAY_LEN(GREETINGS));
}

static void front_window_appear(Window *window) {
  front_load_art();
  layer_mark_dirty(s_front_layer);
}

static void front_window_disappear(Window *window) {
  // 一覧を開いている間は店内の絵を解放する（一覧では顔だけを使う）
  front_unload_art();
}

static void front_window_unload(Window *window) {
  layer_destroy(s_front_layer);
  s_front_layer = NULL;
  front_unload_art();
  window_destroy(window);
  s_front_window = NULL;
}

void shop_window_push(void) {
  if (s_front_window) return;
  s_front_window = window_create();
  window_set_background_color(s_front_window, GColorBlack);
  window_set_click_config_provider(s_front_window, front_click_config);
  window_set_window_handlers(s_front_window, (WindowHandlers){
    .load = front_window_load,
    .appear = front_window_appear,
    .disappear = front_window_disappear,
    .unload = front_window_unload,
  });
  window_stack_push(s_front_window, true);
}

// ============================================================
// 購入・売却の一覧
// ============================================================
#if defined(PBL_ROUND)
#define LIST_HEADER_H SNAP(PBL_DISPLAY_HEIGHT * 34 / 100)
#else
#define LIST_HEADER_H (IS_LARGE_SCREEN ? 58 : 46)
#endif

static void list_header_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  GSize face = s_face ? gbitmap_get_bounds(s_face).size : GSize(0, 0);
#if defined(PBL_ROUND)
  // 丸型：顔を左寄りに置き、セリフは右側に下寄せ（上端は狭いので避ける）
  int face_x = SNAP(b.size.w / 6);
  int x = face_x + face.w + 2 * PX;
  int ty = SNAP(b.size.h / 3);
  GRect r = GRect(x, ty, b.size.w - x - face_x, b.size.h - ty - PX);
#else
  int face_x = 2 * PX;
  int x = face_x + face.w + 3 * PX;
  GRect r = GRect(x, 2 * PX, b.size.w - x - 2 * PX, b.size.h - 3 * PX);
#endif
  int face_y = b.size.h - face.h - 2 * PX;
  if (s_face) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_face, GRect(face_x, face_y, face.w, face.h));
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, b.size.h - PX, b.size.w, PX), 0, GCornerNone);

  gfx_text(ctx, s_speech, r, GTextAlignmentLeft, THEME_FG);
}

// 購入一覧: ポーション、帰還の巻物、装備（勇者のレベルに合わせた品）
#define BUY_POTION 0
#define BUY_PORTAL 1
#define BUY_IDENT 2
#define BUY_GEAR_FIRST 3

static int sell_rows(void) {
  int n = game_bag_count();
  return n > 0 ? n : 1;
}

static uint16_t list_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return s_mode == LIST_BUY ? BUY_GEAR_FIRST + SHOP_GEAR_COUNT : sell_rows();
}

static int16_t list_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return MENU_HEADER_H;
}

static void list_draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "%s %ldG", s_mode == LIST_BUY ? "BUY" : "SELL", (long)game_gold());
  gfx_draw_header(ctx, cell, buf);
}

static int16_t list_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void list_draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  static char sub[32];
  static char right[8];
  RowSpec row = { .icon = -1 };
  if (s_mode == LIST_BUY) {
    if (index->row == BUY_POTION) {
      row.icon = UI_ICON_POTION;
      row.title = "Potion";
      snprintf(sub, sizeof(sub), "%dG Heal 50%%", POTION_PRICE);
      snprintf(right, sizeof(right), "%d/%d", game_potions(), MAX_POTIONS);
      row.warn_sub = game_gold() < POTION_PRICE;
    } else if (index->row == BUY_PORTAL) {
      row.icon = UI_ICON_PORTAL_SCROLL;
      row.title = "Portal Scroll";
      snprintf(sub, sizeof(sub), "%dG To town", PORTAL_PRICE);
      snprintf(right, sizeof(right), "%d/%d", game_portals(), MAX_PORTALS);
      row.warn_sub = game_gold() < PORTAL_PRICE;
    } else if (index->row == BUY_IDENT) {
      row.icon = UI_ICON_IDENTIFY_SCROLL;
      row.title = "Identify Scroll";
      snprintf(sub, sizeof(sub), "%dG Reveals gear", IDENT_SCROLL_PRICE);
      snprintf(right, sizeof(right), "%d/%d", game_identify_scrolls(), MAX_IDENT_SCROLLS);
      row.warn_sub = game_gold() < IDENT_SCROLL_PRICE;
    } else {
      int i = index->row - BUY_GEAR_FIRST;
      static char name[32];
      static Item it;   // 描き終わるまで残しておく
      it = game_shop_gear(i);
      char stats[24];
      gfx_item_stat_text(&it, stats, sizeof(stats));
      int cost = game_shop_gear_cost(i);
      row.item = &it;
      game_item_short_name(&it, name, sizeof(name));
      row.title = name;
      snprintf(sub, sizeof(sub), "%dG %s", cost, stats);
      row.warn_sub = game_gold() < cost;
      right[0] = '\0';
    }
    row.sub = sub;
    if (right[0]) row.right = right;
  } else {
    const Item *it = game_bag(index->row);
    if (!it) {
      row.title = "Nothing to sell";
      row.sub = "Go find loot!";
      row.dim = true;
    } else {
      static char name[40];
      static char stats[24];
      gfx_item_row(&row, it, name, sizeof(name), stats, sizeof(stats));
      snprintf(sub, sizeof(sub), game_item_identified(it) ? "Sell %dG" : "Sell %dG (unident.)",
               game_item_price(it));
      row.sub = sub;
      row.warn_sub = false;
    }
  }
  gfx_draw_row(ctx, cell, &row);
}

static void list_select(MenuLayer *menu, MenuIndex *index, void *data) {
  if (s_mode == LIST_BUY) {
    BuyResult r;
    if (index->row == BUY_POTION) r = game_buy_potion();
    else if (index->row == BUY_PORTAL) r = game_buy_portal();
    else if (index->row == BUY_IDENT) r = game_buy_identify();
    else r = game_buy_gear(index->row - BUY_GEAR_FIRST);
    switch (r) {
      case BUY_OK: s_speech = pick(THANKS, ARRAY_LEN(THANKS)); break;
      case BUY_NO_GOLD: s_speech = "Not enough gold, sweetie."; break;
      case BUY_FULL: s_speech = index->row < BUY_GEAR_FIRST ? "You can't carry more, dear." : "Your bag is full, dear."; break;
    }
  } else {
    if (!game_sell_bag(index->row)) return;
    s_speech = "Ooh, I'll take that~";
    int n = sell_rows();
    if (index->row >= n) {
      menu_layer_set_selected_index(menu, MenuIndex(0, n - 1), MenuRowAlignCenter, false);
    }
  }
  menu_layer_reload_data(menu);
  layer_mark_dirty(s_list_header);
}

// 長押しで品物の詳細（買う前・売る前に中身を確かめる）
static void list_long_select(MenuLayer *menu, MenuIndex *index, void *data) {
  if (s_mode == LIST_BUY) {
    if (index->row < BUY_GEAR_FIRST) return;
    Item it = game_shop_gear(index->row - BUY_GEAR_FIRST);
    item_window_push_copy(&it);
  } else if (game_bag(index->row)) {
    item_window_push(ITEM_AT_BAG, index->row, false);
  }
}

static void list_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  s_face = gfx_keeper_acquire();

  s_list_header = layer_create(GRect(0, 0, b.size.w, LIST_HEADER_H));
  layer_set_update_proc(s_list_header, list_header_update_proc);
  layer_add_child(root, s_list_header);

  s_list_menu = menu_layer_create(GRect(0, LIST_HEADER_H, b.size.w, b.size.h - LIST_HEADER_H));
  menu_layer_set_callbacks(s_list_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = list_num_rows,
    .get_header_height = list_header_height,
    .draw_header = list_draw_header,
    .get_cell_height = list_cell_height,
    .draw_row = list_draw_row,
    .select_click = list_select,
    .select_long_click = list_long_select,
  });
  gfx_setup_menu(s_list_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_list_menu));

  s_speech = (s_mode == LIST_BUY) ? "See anything you like~?" : "What have you got?";
}

static void list_window_unload(Window *window) {
  menu_layer_destroy(s_list_menu);
  s_list_menu = NULL;
  layer_destroy(s_list_header);
  s_list_header = NULL;
  s_face = NULL;            // 実体は gfx 側が持っているので破棄しない
  gfx_keeper_release();
  window_destroy(window);
  s_list_window = NULL;
}

static void list_window_push(ListMode mode) {
  if (s_list_window) return;
  s_mode = mode;
  s_list_window = window_create();
  window_set_window_handlers(s_list_window, (WindowHandlers){
    .load = list_window_load,
    .unload = list_window_unload,
  });
  window_stack_push(s_list_window, true);
}
