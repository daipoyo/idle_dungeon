#include "ui.h"
#include "game.h"
#include "gfx.h"
#include <stdlib.h>

// ============================================================
// お店
//   店頭画面 : 店内の背景・カウンター越しの女主人・会話ウィンドウ
//              上下ボタンで Buy / Sell / Talk を選び、SELECTで決定
//   購入・売却: 女主人の顔とセリフ付きのアイテム一覧
// ============================================================

// 女主人の画像内での右目の位置（ウインク用。tools/art_shopkeeper.py と対応）
#define KEEPER_WINK_X 51
#define KEEPER_WINK_Y 27
#define WINK_INTERVAL_MS 4200
#define WINK_MS 350

typedef enum { OPT_BUY, OPT_SELL, OPT_TALK, OPT_COUNT } ShopOption;
static const char *const OPTION_LABELS[OPT_COUNT] = { "Buy", "Sell", "Talk" };

static const char *const GREETINGS[] = {
  "Welcome, darling~ Take your time.",
  "Oh, it's you again~ I missed you.",
  "Come in, handsome. Looking for something?",
};
static const char *const TALKS[] = {
  "You look strong today~",
  "Come back safe, okay? I'll be waiting.",
  "I saved the best stuff just for you~",
  "Don't stare too long, cutie~",
  "Bring me something shiny, hero?",
  "Brave men are my type, you know~",
};
static const char *const THANKS[] = {
  "Great choice, darling~",
  "Thank you~ It suits you!",
  "Mmm, a man who knows quality~",
};

#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))

static const char *s_speech;

// ---- 店頭 ----
static Window *s_front_window;
static Layer *s_front_layer;
static GBitmap *s_shop_bg, *s_wall, *s_counter;
static GBitmap *s_keeper;
static ShopOption s_option;
static bool s_wink;
static AppTimer *s_wink_timer;

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
// 女主人の描画
// ============================================================
static void draw_wink(GContext *ctx, int kx, int ky) {
  int x = kx + KEEPER_WINK_X;
  int y = ky + KEEPER_WINK_Y;
  graphics_context_set_fill_color(ctx, GColorMelon);
  graphics_fill_rect(ctx, GRect(x, y, 7, 6), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(x, y + 2, 1, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 1, y + 3, 5, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 6, y + 2, 1, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 7, y + 1, 1, 1), 0, GCornerNone);
}

static void wink_timer_cb(void *data);

static void schedule_wink(int ms) {
  if (s_wink_timer) app_timer_cancel(s_wink_timer);
  s_wink_timer = app_timer_register(ms, wink_timer_cb, NULL);
}

static void wink_timer_cb(void *data) {
  s_wink_timer = NULL;
  s_wink = !s_wink;
  if (s_front_layer) layer_mark_dirty(s_front_layer);
  schedule_wink(s_wink ? WINK_MS : WINK_INTERVAL_MS);
}

// ============================================================
// 店頭画面
// ============================================================
static void front_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int w = b.size.w, h = b.size.h;
#if defined(PBL_ROUND)
  int dlg_h = h * 34 / 100;
#else
  int dlg_h = IS_LARGE_SCREEN ? 72 : 56;
#endif
  int counter_y = h - dlg_h - SHOP_COUNTER_H;
  int keeper_y = counter_y - (KEEPER_H - 16);
  int wall_top = counter_y - SHOP_WALL_H;

  graphics_context_set_fill_color(ctx, gfx_argb(BG_SHOP_FILL));
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  // 壁紙とカウンターは中央揃えで横に並べる
  int x0 = (w / 2 - BG_TILE_W / 2) % BG_TILE_W;
  if (x0 > 0) x0 -= BG_TILE_W;
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  if (s_wall) graphics_draw_bitmap_in_rect(ctx, s_wall, GRect(x0, wall_top, w - x0, SHOP_WALL_H));

  int kx = (w - KEEPER_W) / 2;
  if (s_keeper) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_keeper, GRect(kx, keeper_y, KEEPER_W, KEEPER_H));
    if (s_wink) draw_wink(ctx, kx, keeper_y);
  }
  graphics_context_set_compositing_mode(ctx, GCompOpAssign);
  if (s_counter) graphics_draw_bitmap_in_rect(ctx, s_counter, GRect(x0, counter_y, w - x0, SHOP_COUNTER_H));

  // 会話ウィンドウ
  int inset = PBL_IF_ROUND_ELSE(w / 8, 2);
  GRect box = GRect(inset, h - dlg_h + 1, w - inset * 2, dlg_h - PBL_IF_ROUND_ELSE(4, 3));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, h - dlg_h, w, dlg_h), 0, GCornerNone);
  gfx_draw_panel(ctx, box, THEME_BG, GColorWhite);
  gfx_draw_heart(ctx, box.origin.x + box.size.w - 10, box.origin.y + 3);

  int line = IS_LARGE_SCREEN ? 20 : 16;
  GRect speech = GRect(box.origin.x + 4, box.origin.y - 1, box.size.w - 16, line * 2 + 2);
  gfx_draw_text(ctx, s_speech, gfx_font_small(), speech,
                PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft), GColorWhite);

  // 選択肢（丸型は下へ行くほど狭いので中央に寄せる）
  int opt_y = box.origin.y + box.size.h - line - PBL_IF_ROUND_ELSE(10, 4);
  int opt_area = PBL_IF_ROUND_ELSE(w * 6 / 10, box.size.w - 8);
  int opt_x = (w - opt_area) / 2;
  int opt_w = opt_area / OPT_COUNT;
  for (int i = 0; i < OPT_COUNT; i++) {
    GRect r = GRect(opt_x + i * opt_w, opt_y, opt_w - 2, line + 2);
    bool sel = (i == (int)s_option);
    if (sel) {
      graphics_context_set_fill_color(ctx, THEME_HI_BG);
      graphics_fill_rect(ctx, r, 3, GCornersAll);
    }
    gfx_draw_text(ctx, OPTION_LABELS[i], gfx_font_small_bold(),
                  GRect(r.origin.x, r.origin.y - 3, r.size.w, r.size.h + 4),
                  GTextAlignmentCenter, sel ? THEME_HI_FG : GColorWhite);
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
      s_wink = true;
      schedule_wink(WINK_MS * 2);
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
  if (!s_shop_bg) {
    s_shop_bg = gbitmap_create_with_resource(RESOURCE_ID_IMG_BG_SHOP);
    s_wall = gbitmap_create_as_sub_bitmap(s_shop_bg, GRect(0, 0, BG_TILE_W, SHOP_WALL_H));
    s_counter = gbitmap_create_as_sub_bitmap(s_shop_bg, GRect(0, SHOP_WALL_H, BG_TILE_W, SHOP_COUNTER_H));
  }
}

static void front_unload_art(void) {
  if (s_shop_bg) {
    gbitmap_destroy(s_wall);
    gbitmap_destroy(s_counter);
    gbitmap_destroy(s_shop_bg);
    s_wall = s_counter = s_shop_bg = NULL;
  }
}

static void front_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_front_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_front_layer, front_update_proc);
  layer_add_child(root, s_front_layer);
  s_keeper = gfx_keeper_acquire();
  s_option = OPT_BUY;
  s_speech = pick(GREETINGS, ARRAY_LEN(GREETINGS));
}

static void front_window_appear(Window *window) {
  front_load_art();
  s_wink = false;
  schedule_wink(WINK_INTERVAL_MS / 2);
  layer_mark_dirty(s_front_layer);
}

static void front_window_disappear(Window *window) {
  // 一覧を開いている間は店内の背景を解放（女主人の画像は一覧でも使う）
  if (s_wink_timer) {
    app_timer_cancel(s_wink_timer);
    s_wink_timer = NULL;
  }
  s_wink = false;
  front_unload_art();
}

static void front_window_unload(Window *window) {
  layer_destroy(s_front_layer);
  s_front_layer = NULL;
  front_unload_art();
  gfx_keeper_release();
  s_keeper = NULL;
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
#define LIST_HEADER_H (PBL_DISPLAY_HEIGHT * 34 / 100)
#else
#define LIST_HEADER_H (IS_LARGE_SCREEN ? 58 : 46)
#endif
#define FACE_W (IS_LARGE_SCREEN ? 56 : 46)

static void list_header_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorImperialPurple);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
#if defined(PBL_ROUND)
  // 丸型：顔を中央やや左に置き、セリフは右側に下寄せ
  int face_x = b.size.w / 2 - FACE_W - 2;
  int x = b.size.w / 2 + 2;
  GRect r = GRect(x, b.size.h / 5, b.size.w * 3 / 10 + 4, b.size.h - b.size.h / 5 - 2);
#else
  int face_x = 2;
  int x = face_x + FACE_W + 3;
  GRect r = GRect(x, 0, b.size.w - x - 4, b.size.h - 2);
#endif
  if (s_face) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_face, GRect(face_x, b.size.h - (LIST_HEADER_H - 2),
                                                    FACE_W, LIST_HEADER_H - 2));
  }
  graphics_context_set_stroke_color(ctx, GColorChromeYellow);
  graphics_draw_line(ctx, GPoint(0, b.size.h - 1), GPoint(b.size.w, b.size.h - 1));

  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_speech, gfx_font_small(), r, GTextOverflowModeWordWrap,
                     GTextAlignmentLeft, NULL);
}

static int sell_rows(void) {
  int n = game_owned_kinds();
  return n > 0 ? n : 1;
}

static uint16_t list_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  return s_mode == LIST_BUY ? ITEM_COUNT : sell_rows();
}

static int16_t list_header_height(MenuLayer *menu, uint16_t section, void *data) {
  return 18;
}

static void list_draw_header(GContext *ctx, const Layer *cell, uint16_t section, void *data) {
  static char buf[32];
  snprintf(buf, sizeof(buf), "%s  -  Gold %ld", s_mode == LIST_BUY ? "BUY" : "SELL", (long)game_gold());
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
    int id = index->row;
    const ItemDef *it = &g_items[id];
    row.icon = id;
    row.title = it->name;
    if (it->type == ITEM_MATERIAL) {
      snprintf(sub, sizeof(sub), "%dG  Material", it->price);
    } else if (it->atk && it->def) {
      snprintf(sub, sizeof(sub), "%dG  ATK+%d DEF+%d", it->price, it->atk, it->def);
    } else if (it->atk) {
      snprintf(sub, sizeof(sub), "%dG  ATK+%d", it->price, it->atk);
    } else {
      snprintf(sub, sizeof(sub), "%dG  DEF+%d", it->price, it->def);
    }
    row.sub = sub;
    row.warn_sub = game_gold() < it->price;
    if (game_item_count(id) > 0) {
      snprintf(right, sizeof(right), "x%d", game_item_count(id));
      row.right = right;
    }
  } else {
    int id = game_owned_nth(index->row);
    if (id < 0) {
      row.title = "Nothing to sell";
      row.sub = "Go find some loot!";
      row.dim = true;
    } else {
      row.icon = id;
      row.title = g_items[id].name;
      bool locked = game_is_equipped(id) && game_item_count(id) <= 1;
      if (locked) {
        snprintf(sub, sizeof(sub), "Equipped");
      } else {
        snprintf(sub, sizeof(sub), "Sell for %dG", game_sell_price(id));
      }
      row.sub = sub;
      row.dim = locked;
      snprintf(right, sizeof(right), "x%d", game_item_count(id));
      row.right = right;
    }
  }
  gfx_draw_row(ctx, cell, &row);
}

static void list_select(MenuLayer *menu, MenuIndex *index, void *data) {
  if (s_mode == LIST_BUY) {
    switch (game_buy(index->row)) {
      case BUY_OK: s_speech = pick(THANKS, ARRAY_LEN(THANKS)); break;
      case BUY_NO_GOLD: s_speech = "Not enough gold, sweetie."; break;
      case BUY_FULL: s_speech = "Your bag is full, dear."; break;
    }
  } else {
    int id = game_owned_nth(index->row);
    if (id < 0) return;
    switch (game_sell(id)) {
      case SELL_OK: s_speech = "Ooh, I'll take that~"; break;
      case SELL_EQUIPPED: s_speech = "You're wearing that, silly~"; break;
      case SELL_NONE: break;
    }
    int n = sell_rows();
    if (index->row >= n) {
      menu_layer_set_selected_index(menu, MenuIndex(0, n - 1), MenuRowAlignCenter, false);
    }
  }
  menu_layer_reload_data(menu);
  layer_mark_dirty(s_list_header);
}

static void list_window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  gfx_items_acquire();
  GBitmap *keeper = gfx_keeper_acquire();
  // 顔のあたりを切り出して使う
  int crop_h = LIST_HEADER_H - 2;
  s_face = gbitmap_create_as_sub_bitmap(keeper, GRect(KEEPER_W / 2 - FACE_W / 2, 1, FACE_W, crop_h));

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
  });
  gfx_setup_menu(s_list_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_list_menu));

  s_speech = (s_mode == LIST_BUY) ? "See anything you like~?" : "What have you got for me?";
}

static void list_window_unload(Window *window) {
  menu_layer_destroy(s_list_menu);
  s_list_menu = NULL;
  layer_destroy(s_list_header);
  s_list_header = NULL;
  gbitmap_destroy(s_face);
  s_face = NULL;
  gfx_keeper_release();
  gfx_items_release();
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
