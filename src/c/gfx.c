#include "gfx.h"
#include "game.h"
#include "sprite_data.h"

// ============================================================
// フォント
// ============================================================
GFont gfx_font_small(void) {
  return fonts_get_system_font(IS_LARGE_SCREEN ? FONT_KEY_GOTHIC_18 : FONT_KEY_GOTHIC_14);
}

GFont gfx_font_small_bold(void) {
  return fonts_get_system_font(IS_LARGE_SCREEN ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD);
}

GFont gfx_font_title(void) {
  return fonts_get_system_font(IS_LARGE_SCREEN ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_18_BOLD);
}

// ============================================================
// 文字マップ描画（同じ色が続く部分はまとめて塗る）
// ============================================================
void gfx_draw_charmap(GContext *ctx, const char *const *rows, int w, int h,
                      int x, int y, int scale, bool flip, const uint8_t *lut) {
  if (!lut) lut = PAL_LUT;
  for (int r = 0; r < h; r++) {
    const char *row = rows[r];
    int c = 0;
    while (c < w) {
      char ch = row[c];
      int run = 1;
      while (c + run < w && row[c + run] == ch) run++;
      uint8_t argb = ((unsigned char)ch < 128) ? lut[(int)ch] : 0;
      if (argb) {
        int px = flip ? (w - c - run) : c;
        graphics_context_set_fill_color(ctx, gfx_argb(argb));
        graphics_fill_rect(ctx, GRect(x + px * scale, y + r * scale, run * scale, scale), 0, GCornerNone);
      }
      c += run;
    }
  }
}

// ============================================================
// 勇者
// ============================================================
// 防具ごとの色（主・影・ハイライト）。0番は防具なしの青い服
static const char ARMOR_COLORS[5][3] = {
  { 'B', 'N', 'u' },
  { 's', 'o', 'l' },  // Cloth Armor
  { 'o', 'b', 's' },  // Leather Armor
  { 'g', 'k', 'W' },  // Chain Mail
  { 'I', 'i', 'W' },  // Plate Armor
};

// 武器ごとの色（刀身・刀身ハイライト・柄）
static const char WEAPON_COLORS[4][3] = {
  { 'v', 'g', 'b' },  // Rusty Sword
  { 'g', 'W', 'o' },  // Iron Sword
  { 'C', 'W', 'B' },  // Steel Blade
  { 'g', 'W', 'o' },  // Battle Axe
};

static void set_lut(uint8_t *lut, char key, char color) {
  lut[(int)key] = PAL_LUT[(int)color];
}

void gfx_draw_hero(GContext *ctx, int x, int y, HeroPose pose, int scale, bool flip) {
  uint8_t lut[128];
  memcpy(lut, PAL_LUT, sizeof(lut));
  set_lut(lut, 'h', 'o');  // 髪
  set_lut(lut, 'H', 'b');
  set_lut(lut, 'c', 'r');  // マント
  set_lut(lut, 'C', 'b');
  set_lut(lut, 'p', 'N');  // ズボン
  set_lut(lut, 'B', 'b');  // ブーツ

  int armor = game_equipped(ITEM_ARMOR);
  int ai = (armor >= 4 && armor <= 7) ? armor - 3 : 0;
  set_lut(lut, '1', ARMOR_COLORS[ai][0]);
  set_lut(lut, '2', ARMOR_COLORS[ai][1]);
  set_lut(lut, '3', ARMOR_COLORS[ai][2]);

  const char *const *body;
  int hand_x, hand_y;
  switch (pose) {
    case HERO_POSE_WALK0: body = HERO_WALK0; hand_x = HERO_WALK0_HAND_X; hand_y = HERO_WALK0_HAND_Y; break;
    case HERO_POSE_WALK1: body = HERO_WALK1; hand_x = HERO_WALK1_HAND_X; hand_y = HERO_WALK1_HAND_Y; break;
    case HERO_POSE_ATTACK: body = HERO_ATTACK; hand_x = HERO_ATTACK_HAND_X; hand_y = HERO_ATTACK_HAND_Y; break;
    default: body = HERO_IDLE; hand_x = HERO_IDLE_HAND_X; hand_y = HERO_IDLE_HAND_Y; break;
  }
  gfx_draw_charmap(ctx, body, HERO_W, HERO_H, x, y, scale, flip, lut);

  int weapon = game_equipped(ITEM_WEAPON);
  if (weapon < 0 || weapon > 3) return;
  set_lut(lut, '4', WEAPON_COLORS[weapon][0]);
  set_lut(lut, '5', WEAPON_COLORS[weapon][1]);
  set_lut(lut, '6', WEAPON_COLORS[weapon][2]);

  bool swing = (pose == HERO_POSE_ATTACK);
  const char *const *map;
  int w, h, ax, ay;
  if (weapon == 3) {
    if (swing) { map = WPN_AXE_SWING; w = WPN_AXE_SWING_W; h = WPN_AXE_SWING_H; ax = WPN_AXE_SWING_AX; ay = WPN_AXE_SWING_AY; }
    else { map = WPN_AXE_REST; w = WPN_AXE_REST_W; h = WPN_AXE_REST_H; ax = WPN_AXE_REST_AX; ay = WPN_AXE_REST_AY; }
  } else {
    if (swing) { map = WPN_SWORD_SWING; w = WPN_SWORD_SWING_W; h = WPN_SWORD_SWING_H; ax = WPN_SWORD_SWING_AX; ay = WPN_SWORD_SWING_AY; }
    else { map = WPN_SWORD_REST; w = WPN_SWORD_REST_W; h = WPN_SWORD_REST_H; ax = WPN_SWORD_REST_AX; ay = WPN_SWORD_REST_AY; }
  }
  // 手の位置に武器の握りを合わせる（左右反転時は両方を反転して計算）
  int hx = flip ? (HERO_W - 1 - hand_x) : hand_x;
  int wax = flip ? (w - 1 - ax) : ax;
  gfx_draw_charmap(ctx, map, w, h, x + (hx - wax) * scale, y + (hand_y - ay) * scale, scale, flip, lut);
}

// ============================================================
// 共有ビットマップ
// ============================================================
static GBitmap *s_items_sheet;
static GBitmap *s_item_sub;
static int s_items_refs;

void gfx_items_acquire(void) {
  if (s_items_refs++ == 0) {
    s_items_sheet = gbitmap_create_with_resource(RESOURCE_ID_IMG_ITEMS);
    s_item_sub = gbitmap_create_as_sub_bitmap(s_items_sheet, GRect(0, 0, 16, 16));
  }
}

void gfx_items_release(void) {
  if (s_items_refs <= 0) return;
  if (--s_items_refs == 0) {
    gbitmap_destroy(s_item_sub);
    gbitmap_destroy(s_items_sheet);
    s_item_sub = NULL;
    s_items_sheet = NULL;
  }
}

void gfx_draw_item_icon(GContext *ctx, int item_id, int x, int y) {
  if (!s_item_sub || item_id < 0 || item_id >= ITEM_COUNT) return;
  gbitmap_set_bounds(s_item_sub, GRect((item_id % 4) * 16, (item_id / 4) * 16, 16, 16));
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_item_sub, GRect(x, y, 16, 16));
}

uint32_t gfx_enemy_resource(int dungeon) {
  static const uint32_t ids[DUNGEON_COUNT] = {
    RESOURCE_ID_IMG_ENEMY0, RESOURCE_ID_IMG_ENEMY1, RESOURCE_ID_IMG_ENEMY2,
    RESOURCE_ID_IMG_ENEMY3, RESOURCE_ID_IMG_ENEMY4, RESOURCE_ID_IMG_ENEMY5,
  };
  return ids[dungeon];
}

static GBitmap *s_keeper;
static int s_keeper_refs;

GBitmap *gfx_keeper_acquire(void) {
  if (s_keeper_refs++ == 0) {
    s_keeper = gbitmap_create_with_resource(RESOURCE_ID_IMG_KEEPER_FACE);
  }
  return s_keeper;
}

void gfx_keeper_release(void) {
  if (s_keeper_refs <= 0) return;
  if (--s_keeper_refs == 0) {
    gbitmap_destroy(s_keeper);
    s_keeper = NULL;
  }
}

// ============================================================
// メニューの行
// ============================================================
void gfx_setup_menu(MenuLayer *menu, Window *window) {
  menu_layer_set_normal_colors(menu, THEME_BG, THEME_FG);
  menu_layer_set_highlight_colors(menu, THEME_HI_BG, THEME_HI_FG);
  menu_layer_set_click_config_onto_window(menu, window);
  window_set_background_color(window, THEME_BG);
}

void gfx_draw_row(GContext *ctx, const Layer *cell, const RowSpec *row) {
  GRect b = layer_get_bounds(cell);
  bool hi = menu_cell_layer_is_highlighted(cell);
  graphics_context_set_fill_color(ctx, hi ? THEME_HI_BG : THEME_BG);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  int x = PBL_IF_ROUND_ELSE(b.size.w / 10, 4);
  int right_edge = b.size.w - PBL_IF_ROUND_ELSE(b.size.w / 10, 4);
  if (row->icon >= 0) {
    int iy = (b.size.h - 20) / 2;
    graphics_context_set_fill_color(ctx, row->dim ? GColorDarkGray : GColorWhite);
    graphics_fill_rect(ctx, GRect(x, iy, 20, 20), 3, GCornersAll);
    gfx_draw_item_icon(ctx, row->icon, x + 2, iy + 2);
    x += 25;
  } else if (row->bitmap) {
    int iy = (b.size.h - 28) / 2;
    graphics_context_set_fill_color(ctx, row->dim ? GColorDarkGray : GColorWhite);
    graphics_fill_rect(ctx, GRect(x, iy, 28, 28), 3, GCornersAll);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, row->bitmap, GRect(x + 2, iy + 2, 24, 24));
    x += 33;
  }

  GColor fg = hi ? THEME_HI_FG : (row->dim ? GColorLightGray : THEME_FG);
  GColor sub = hi ? GColorBlack : (row->dim ? GColorDarkGray : THEME_SUB);
  if (row->warn_sub) sub = hi ? GColorDarkCandyAppleRed : GColorMelon;

  GFont tf = gfx_font_small_bold();
  GFont sf = gfx_font_small();
  int line = IS_LARGE_SCREEN ? 21 : 17;
  int ty = (b.size.h - line * 2) / 2 - 3;
  int right_w = 0;
  if (row->right) {
    GSize sz = graphics_text_layout_get_content_size(row->right, tf, GRect(0, 0, 60, 30),
                                                      GTextOverflowModeFill, GTextAlignmentRight);
    right_w = sz.w + 4;
    graphics_context_set_text_color(ctx, fg);
    graphics_draw_text(ctx, row->right, tf, GRect(right_edge - right_w, ty, right_w, line + 4),
                       GTextOverflowModeFill, GTextAlignmentRight, NULL);
  }
  graphics_context_set_text_color(ctx, fg);
  graphics_draw_text(ctx, row->title, tf, GRect(x, ty, right_edge - x - right_w, line + 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if (row->sub) {
    graphics_context_set_text_color(ctx, sub);
    graphics_draw_text(ctx, row->sub, sf, GRect(x, ty + line, right_edge - x, line + 4),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

void gfx_draw_header(GContext *ctx, const Layer *cell, const char *text) {
  GRect b = layer_get_bounds(cell);
  graphics_context_set_fill_color(ctx, THEME_HEADER_BG);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorChromeYellow);
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(0, -2, b.size.w, b.size.h + 2), GTextOverflowModeFill,
                     GTextAlignmentCenter, NULL);
}

// ============================================================
// 小物
// ============================================================
void gfx_draw_coin(GContext *ctx, int cx, int cy) {
  graphics_context_set_fill_color(ctx, GColorWindsorTan);
  graphics_fill_circle(ctx, GPoint(cx, cy), 4);
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_circle(ctx, GPoint(cx, cy), 3);
  graphics_context_set_fill_color(ctx, GColorIcterine);
  graphics_fill_rect(ctx, GRect(cx - 1, cy - 2, 2, 2), 0, GCornerNone);
}

static const char *const HEART[] = {
  ".RR.RR.",
  "RSRRRRR",
  "RRRRRRR",
  ".RRRRR.",
  "..RRR..",
  "...R...",
};

void gfx_draw_heart(GContext *ctx, int x, int y) {
  gfx_draw_charmap(ctx, HEART, 7, 6, x, y, 1, false, NULL);
}

void gfx_draw_panel(GContext *ctx, GRect r, GColor fill, GColor border) {
  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_rect(ctx, r, 4, GCornersAll);
  graphics_context_set_stroke_color(ctx, border);
  graphics_draw_round_rect(ctx, r, 4);
}

void gfx_draw_text(GContext *ctx, const char *text, GFont font, GRect r,
                   GTextAlignment align, GColor color) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, r, GTextOverflowModeWordWrap, align, NULL);
}

void gfx_draw_shadow_text(GContext *ctx, const char *text, GFont font, GRect r,
                          GTextAlignment align, GColor color) {
  graphics_context_set_text_color(ctx, GColorBlack);
  for (int i = 0; i < 4; i++) {
    int dx = (i == 0) ? -1 : (i == 1) ? 1 : 0;
    int dy = (i == 2) ? -1 : (i == 3) ? 1 : 0;
    graphics_draw_text(ctx, text, font, GRect(r.origin.x + dx, r.origin.y + dy, r.size.w, r.size.h),
                       GTextOverflowModeWordWrap, align, NULL);
  }
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, r, GTextOverflowModeWordWrap, align, NULL);
}
