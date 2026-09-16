#include "gfx.h"
#include "game.h"
#include "sprite_data.h"
#include "font_data.h"

// ============================================================
// ドットフォント
// ============================================================
static const uint8_t *glyph(char c) {
  unsigned char u = (unsigned char)c;
  if (u >= 'a' && u <= 'z') u -= 'a' - 'A';
  if (u < FONT_FIRST || u > FONT_LAST) u = '?';
  return FONT_GLYPHS[u - FONT_FIRST];
}

// 文字の送り幅（字間1ドットを含む）
static int advance(char c) {
  return (glyph(c)[0] + 1) * PX;
}

static int span_width(const char *s, int n) {
  int w = 0;
  for (int i = 0; i < n; i++) w += advance(s[i]);
  return n > 0 ? w - PX : 0;
}

// s から始まる1行の文字数を返し、*next に次の行の先頭を入れる
static int next_line(const char *s, int max_w, const char **next) {
  int w = 0;
  int last_space = -1;
  int i = 0;
  for (; s[i] && s[i] != '\n'; i++) {
    if (s[i] == ' ') last_space = i;
    int nw = w + advance(s[i]);
    if (i > 0 && nw - PX > max_w) {
      if (last_space > 0) {
        *next = s + last_space + 1;
        return last_space;
      }
      *next = s + i;
      return i;
    }
    w = nw;
  }
  *next = (s[i] == '\n') ? s + i + 1 : s + i;
  return i;
}

static void draw_span(GContext *ctx, const char *s, int n, int x, int y) {
  for (int i = 0; i < n; i++) {
    const uint8_t *g = glyph(s[i]);
    for (int r = 0; r < FONT_GLYPH_H; r++) {
      uint8_t bits = g[1 + r];
      int c = 0;
      while (bits >> c) {
        if (!((bits >> c) & 1)) { c++; continue; }
        int run = 1;
        while ((bits >> (c + run)) & 1) run++;
        graphics_fill_rect(ctx, GRect(x + c * PX, y + r * PX, run * PX, PX), 0, GCornerNone);
        c += run;
      }
    }
    x += advance(s[i]);
  }
}

int gfx_text_width(const char *text) {
  int n = 0;
  while (text[n] && text[n] != '\n') n++;
  return span_width(text, n);
}

int gfx_text_lines(const char *text, int width) {
  int lines = 0;
  const char *p = text;
  while (*p) {
    const char *next;
    next_line(p, width, &next);
    p = next;
    lines++;
  }
  return lines;
}

int gfx_text(GContext *ctx, const char *text, GRect box, GTextAlignment align, GColor color) {
  int max_lines = box.size.h / LINE_H;
  if (max_lines < 1) max_lines = 1;
  graphics_context_set_fill_color(ctx, color);
  int lines = 0;
  const char *p = text;
  while (*p && lines < max_lines) {
    const char *next;
    int n = next_line(p, box.size.w, &next);
    // 最後の行に入りきらないときは、単語の途中でも入るところまで描く
    if (lines == max_lines - 1 && *next && next[-1] != '\n') {
      n = 0;
      while (p[n] && p[n] != '\n' && span_width(p, n + 1) <= box.size.w) n++;
    }
    int w = span_width(p, n);
    int x = box.origin.x;
    if (align == GTextAlignmentCenter) x += SNAP((box.size.w - w) / 2);
    else if (align == GTextAlignmentRight) x += box.size.w - w;
    draw_span(ctx, p, n, x, box.origin.y + lines * LINE_H);
    p = next;
    lines++;
  }
  return lines;
}

void gfx_text_outlined(GContext *ctx, const char *text, GRect box, GTextAlignment align,
                       GColor color, GColor outline) {
  static const int8_t OFFS[8][2] = {
    { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 },
  };
  for (int i = 0; i < 8; i++) {
    GRect r = GRect(box.origin.x + OFFS[i][0] * PX, box.origin.y + OFFS[i][1] * PX,
                    box.size.w, box.size.h);
    gfx_text(ctx, text, r, align, outline);
  }
  gfx_text(ctx, text, box, align, color);
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
// 防具ごとの服の色（主・影）。0番は防具なしの緑の服
static const char ARMOR_COLORS[5][2] = {
  { 'G', 'd' },
  { 'l', 'A' },  // Cloth Armor
  { 'o', 'b' },  // Leather Armor
  { 'g', 'k' },  // Chain Mail
  { 'I', 'i' },  // Plate Armor
};

// 武器ごとの色（刀身・刀身ハイライト・柄）
static const char WEAPON_COLORS[4][3] = {
  { 'v', 'e', 'b' },  // Rusty Sword
  { 'g', 'W', 'o' },  // Iron Sword
  { 'u', 'C', 'B' },  // Steel Blade
  { 'g', 'W', 'o' },  // Battle Axe
};

static void set_lut(uint8_t *lut, char key, char color) {
  lut[(int)key] = PAL_LUT[(int)color];
}

void gfx_draw_hero(GContext *ctx, int x, int y, HeroPose pose, int scale, bool flip) {
  uint8_t lut[128];
  memcpy(lut, PAL_LUT, sizeof(lut));
  set_lut(lut, 'h', 'o');  // 髪
  set_lut(lut, 'e', 's');  // 肌

  // 体の防具のアイテムレベルで服の色を変える（なしは緑の服）
  const Item *armor = game_equipped(SLOT_BODY);
  int ai = 0;
  if (armor) ai = armor->ilvl < 10 ? 1 : (armor->ilvl < 22 ? 2 : (armor->ilvl < 35 ? 3 : 4));
  set_lut(lut, '1', ARMOR_COLORS[ai][0]);
  set_lut(lut, '2', ARMOR_COLORS[ai][1]);

  const char *const *body;
  int hand_x, hand_y;
  switch (pose) {
    case HERO_POSE_WALK0: body = HERO_WALK0; hand_x = HERO_WALK0_HAND_X; hand_y = HERO_WALK0_HAND_Y; break;
    case HERO_POSE_WALK1: body = HERO_WALK1; hand_x = HERO_WALK1_HAND_X; hand_y = HERO_WALK1_HAND_Y; break;
    case HERO_POSE_ATTACK: body = HERO_ATTACK; hand_x = HERO_ATTACK_HAND_X; hand_y = HERO_ATTACK_HAND_Y; break;
    default: body = HERO_IDLE; hand_x = HERO_IDLE_HAND_X; hand_y = HERO_IDLE_HAND_Y; break;
  }
  gfx_draw_charmap(ctx, body, HERO_MAP_W, HERO_MAP_H, x, y, scale, flip, lut);

  // 武器もアイテムレベルで見た目を変える（錆びた剣 → 鉄 → 鋼 → 斧）
  const Item *wpn = game_equipped(SLOT_WEAPON);
  if (!wpn) return;
  int weapon = wpn->ilvl < 5 ? 0 : (wpn->ilvl < 18 ? 1 : (wpn->ilvl < 35 ? 2 : 3));
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
  int hx = flip ? (HERO_MAP_W - 1 - hand_x) : hand_x;
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
    s_item_sub = gbitmap_create_as_sub_bitmap(s_items_sheet, GRect(0, 0, ICON_SIZE, ICON_SIZE));
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
  if (!s_item_sub || item_id < 0 || item_id >= 16) return;
  gbitmap_set_bounds(s_item_sub, GRect((item_id % 4) * ICON_SIZE, (item_id / 4) * ICON_SIZE,
                                       ICON_SIZE, ICON_SIZE));
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_item_sub, GRect(x, y, ICON_SIZE, ICON_SIZE));
}

void gfx_draw_item(GContext *ctx, const Item *it, int x, int y) {
  const ShapeDef *sh = game_item_shape(it);
  if (sh) gfx_draw_item_icon(ctx, sh->icon, x, y);
}

GColor gfx_rarity_color(const Item *it) {
  if (!it) return THEME_FG;
  switch (it->rarity) {
    case RARITY_MAGIC: return GColorPictonBlue;
    case RARITY_RARE: return GColorIcterine;
    case RARITY_SET: return GColorBrightGreen;
    case RARITY_UNIQUE: return GColorChromeYellow;
    default: return THEME_FG;
  }
}

void gfx_item_row(RowSpec *row, const Item *it, char *name, size_t name_size, char *sub,
                  size_t sub_size) {
  const ShapeDef *sh = game_item_shape(it);
  if (!sh) return;
  row->icon = sh->icon;
  // 一覧は短い名前。接辞まで含めた名前は詳細画面で出す
  game_item_short_name(it, name, name_size);
  row->title = name;
  static char plus[6];
  if (it->plus) {
    snprintf(plus, sizeof(plus), "+%d", it->plus);
    row->right = plus;
  }
  row->tint_title = true;
  row->title_color = gfx_rarity_color(it);
  gfx_item_stat_text(it, sub, sub_size);
  row->sub = sub;
  row->warn_sub = !game_item_identified(it);
}

void gfx_item_stat_text(const Item *it, char *buf, size_t size) {
  if (it && !game_item_identified(it)) {
    snprintf(buf, size, "Unidentified");
    return;
  }
  ItemStats s = game_item_stats(it);
  int n = 0;
  buf[0] = '\0';
  if (s.atk) n += snprintf(buf + n, size - n, "ATK+%d ", s.atk);
  if (s.def && n < (int)size) n += snprintf(buf + n, size - n, "DEF+%d ", s.def);
  if (s.hp && n < (int)size) n += snprintf(buf + n, size - n, "HP+%d ", s.hp);
  if (n > 0 && n <= (int)size) buf[n - 1] = '\0';   // 末尾の空白を消す
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
  menu_layer_set_highlight_colors(menu, THEME_BG, THEME_HI);
  menu_layer_set_click_config_onto_window(menu, window);
  window_set_background_color(window, THEME_BG);
}

void gfx_draw_row(GContext *ctx, const Layer *cell, const RowSpec *row) {
  GRect b = layer_get_bounds(cell);
  bool hi = menu_cell_layer_is_highlighted(cell);
  graphics_context_set_fill_color(ctx, THEME_BG);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  int x = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 10, 2));
  int right_edge = b.size.w - PBL_IF_ROUND_ELSE(b.size.w / 10, 4);
  int two_lines = row->sub ? LINE_H + TEXT_H : TEXT_H;
  int ty = SNAP((b.size.h - two_lines) / 2);

  // カーソル
  if (hi) gfx_draw_cursor(ctx, x, ty, THEME_HI);
  x += 4 * PX;

  if (row->icon >= 0) {
    gfx_draw_item_icon(ctx, row->icon, x, SNAP((b.size.h - ICON_SIZE) / 2));
    x += ICON_SIZE + 2 * PX;
  } else if (row->bitmap) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, row->bitmap,
                                 GRect(x, SNAP((b.size.h - ENEMY_SIZE) / 2), ENEMY_SIZE, ENEMY_SIZE));
    x += ENEMY_SIZE + 2 * PX;
  }

  GColor fg = row->dim ? THEME_DIM : (hi ? THEME_HI : THEME_FG);
  GColor sub = row->dim ? THEME_DIM : THEME_SUB;
  if (row->warn_sub) sub = THEME_WARN;
  // レア度の色は、選んでいない行のタイトルにだけ使う（選択中は反転して見えるため）
  GColor title_fg = (row->tint_title && !row->dim && !hi) ? row->title_color : fg;

  int right_w = 0;
  if (row->right) {
    right_w = gfx_text_width(row->right) + 2 * PX;
    gfx_text(ctx, row->right, GRect(right_edge - right_w, ty, right_w, LINE_H),
             GTextAlignmentRight, fg);
  }
  gfx_text(ctx, row->title, GRect(x, ty, right_edge - x - right_w, LINE_H), GTextAlignmentLeft, title_fg);
  if (row->sub) {
    gfx_text(ctx, row->sub, GRect(x, ty + LINE_H, right_edge - x, LINE_H), GTextAlignmentLeft, sub);
  }
}

void gfx_draw_header(GContext *ctx, const Layer *cell, const char *text) {
  GRect b = layer_get_bounds(cell);
  graphics_context_set_fill_color(ctx, THEME_BG);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  int w = gfx_text_width(text);
  int tx = SNAP((b.size.w - w) / 2);
  int ty = SNAP((b.size.h - TEXT_H) / 2);
  // 文字の左右に線を引く
  int inset = PBL_IF_ROUND_ELSE(b.size.w / 6, 4);
  int ly = ty + 2 * PX;
  graphics_context_set_fill_color(ctx, THEME_DIM);
  if (tx - 3 * PX > inset) {
    graphics_fill_rect(ctx, GRect(inset, ly, tx - 2 * PX - inset, PX), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(tx + w + 2 * PX, ly, b.size.w - inset - (tx + w + 2 * PX), PX), 0, GCornerNone);
  }
  gfx_text(ctx, text, GRect(tx, ty, w + PX, LINE_H), GTextAlignmentLeft, THEME_GOLD);
}

// ============================================================
// 小物
// ============================================================
static const char *const COIN[] = {
  ".yyy.",
  "yYYyo",
  "yYyyo",
  "yyyyo",
  ".ooo.",
};

void gfx_draw_coin(GContext *ctx, int x, int y) {
  gfx_draw_charmap(ctx, COIN, 5, 5, x, y, PX, false, NULL);
}

void gfx_draw_heart(GContext *ctx, int x, int y) {
  gfx_text(ctx, "{", GRect(x, y, 4 * PX, LINE_H), GTextAlignmentLeft, GColorFolly);
}

void gfx_draw_cursor(GContext *ctx, int x, int y, GColor color) {
  gfx_text(ctx, "}", GRect(x, y, 4 * PX, LINE_H), GTextAlignmentLeft, color);
}

static const char *const SPARK[] = {
  "W...W",
  ".Y.Y.",
  "..W..",
  ".Y.Y.",
  "W...W",
};

void gfx_draw_spark(GContext *ctx, int cx, int cy) {
  gfx_draw_charmap(ctx, SPARK, 5, 5, SNAP(cx - 5 * PX / 2), SNAP(cy - 5 * PX / 2), PX, false, NULL);
}

static const char *const POOF[2][7] = {
  {
    ".......",
    "..W.W..",
    ".WWWWW.",
    "..WgW..",
    ".WWWWW.",
    "..W.W..",
    ".......",
  },
  {
    "g..g..g",
    ".g...g.",
    "...g...",
    "gg...gg",
    "...g...",
    ".g...g.",
    "g..g..g",
  },
};

void gfx_draw_poof(GContext *ctx, int cx, int cy, int frame) {
  gfx_draw_charmap(ctx, POOF[frame ? 1 : 0], 7, 7, SNAP(cx - 7 * PX / 2), SNAP(cy - 7 * PX / 2),
                   PX, false, NULL);
}

void gfx_draw_window(GContext *ctx, GRect r) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, r, 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  int x0 = r.origin.x + PX, y0 = r.origin.y + PX;
  int w = r.size.w - 2 * PX, h = r.size.h - 2 * PX;
  graphics_fill_rect(ctx, GRect(x0 + PX, y0, w - 2 * PX, PX), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x0 + PX, y0 + h - PX, w - 2 * PX, PX), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x0, y0 + PX, PX, h - 2 * PX), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x0 + w - PX, y0 + PX, PX, h - 2 * PX), 0, GCornerNone);
}

GRect gfx_window_inner(GRect r) {
  return GRect(r.origin.x + 3 * PX, r.origin.y + 3 * PX, r.size.w - 6 * PX, r.size.h - 6 * PX);
}
