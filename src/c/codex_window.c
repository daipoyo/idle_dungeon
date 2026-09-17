#include "ui.h"
#include "game.h"
#include "gfx.h"

// ============================================================
// 図鑑（マス目表示）
//   装備枠ごと（武器・盾…）と、固有装備・セット装備に区切って、アイコンを並べる。
//   基本アイテムは 1行 = 1つの形の素材6段階。まだ手に入れていない物は黒い影。
//   UP / DOWN: 1マスずつ（押し続けると連続）  SELECT: 詳細  SELECT 長押し: 次の区切りへ
// ============================================================
#define SECTION_MAX 10
#define LABEL_H (LINE_H + 2 * PX)
#define FOOTER_H (2 * LINE_H + 3 * PX)

typedef struct {
  const char *label;
  int first;   // 図鑑の番号
  int count;
} Section;

static Window *s_window;
static Layer *s_layer;
static Section s_sections[SECTION_MAX];
static int s_section_count;
static int s_cursor;   // 図鑑の番号
static int s_scroll;   // マス目の表示位置（ピクセル）
static int s_cols;
static int s_cell;

static void build_sections(void) {
  static const struct {
    uint8_t slot;
    const char *label;
  } SLOTS[] = {
    { SLOT_WEAPON, "WEAPONS" }, { SLOT_OFFHAND, "SHIELDS" }, { SLOT_HEAD, "HEAD" },
    { SLOT_BODY, "BODY" },      { SLOT_HANDS, "HANDS" },     { SLOT_FEET, "FEET" },
    { SLOT_AMULET, "AMULETS" }, { SLOT_RING1, "RINGS" },
  };
  s_section_count = 0;
  for (unsigned k = 0; k < sizeof(SLOTS) / sizeof(SLOTS[0]); k++) {
    int first = -1, count = 0;
    for (int shape = 0; shape < SHAPE_COUNT; shape++) {
      if (g_shapes[shape].slot != SLOTS[k].slot) continue;
      if (first < 0) first = shape;
      count++;
    }
    if (first < 0) continue;
    s_sections[s_section_count++] = (Section){ SLOTS[k].label, first * TIER_COUNT, count * TIER_COUNT };
  }
  // 固有装備が先、セット装備が後に並んでいる
  int uniques = 0;
  while (uniques < g_special_count && g_specials[uniques].set_id == 0) uniques++;
  s_sections[s_section_count++] = (Section){ "UNIQUES", BASE_COUNT, uniques };
  s_sections[s_section_count++] = (Section){ "SETS", BASE_COUNT + uniques, g_special_count - uniques };
}

static int section_height(const Section *sec) {
  int rows = (sec->count + s_cols - 1) / s_cols;
  return LABEL_H + rows * s_cell + PX;
}

static int section_of(int entry) {
  for (int k = 0; k < s_section_count; k++) {
    if (entry < s_sections[k].first + s_sections[k].count) return k;
  }
  return s_section_count - 1;
}

// マス目の中での、その番号のマスの上端（ピクセル。スクロール前）
static int entry_y(int entry) {
  int y = 0;
  int k = section_of(entry);
  for (int i = 0; i < k; i++) y += section_height(&s_sections[i]);
  return y + LABEL_H + ((entry - s_sections[k].first) / s_cols) * s_cell;
}

// 見本として描くアイテム（素材の段階に合ったレベル、固有装備はそのダンジョンの深さ）
static Item probe_item(int entry) {
  Item it = { 0 };
  it.base = (uint16_t)(entry + 1);
  it.flags = ITEM_FLAG_IDENTIFIED;
  it.seed = 15;   // 性能のばらつきがちょうど 100%
  if (entry < BASE_COUNT) {
    static const uint8_t TIER_LEVEL[TIER_COUNT] = { 8, 16, 25, 34, 42, 50 };
    it.ilvl = TIER_LEVEL[entry % TIER_COUNT];
    it.rarity = RARITY_NORMAL;
  } else {
    const SpecialDef *d = &g_specials[entry - BASE_COUNT];
    it.rarity = d->set_id ? RARITY_SET : RARITY_UNIQUE;
    it.ilvl = d->dungeon < DUNGEON_COUNT ? g_dungeons[d->dungeon].lvl_max : 30;
  }
  return it;
}

static int seen_in(const Section *sec) {
  int n = 0;
  for (int i = 0; i < sec->count; i++) n += game_codex_seen(sec->first + i) ? 1 : 0;
  return n;
}

static void draw_rule(GContext *ctx, int y, int w) {
  graphics_context_set_fill_color(ctx, THEME_DIM);
  graphics_fill_rect(ctx, GRect(0, y, w, PX), 0, GCornerNone);
}

static void update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  static char buf[40];

  int grid_top = MENU_HEADER_H;
  int grid_bot = b.size.h - FOOTER_H;
  int x0 = (b.size.w - s_cols * s_cell) / 2;

  // マス目（見出しと下の欄は、あとから上に重ねて描く）
  int y = grid_top - s_scroll;
  for (int k = 0; k < s_section_count; k++) {
    const Section *sec = &s_sections[k];
    int h = section_height(sec);
    if (y + h > grid_top && y < grid_bot) {
      snprintf(buf, sizeof(buf), "%s %d/%d", sec->label, seen_in(sec), sec->count);
      gfx_text(ctx, buf, GRect(x0, y + PX, s_cols * s_cell, LINE_H), GTextAlignmentLeft,
               k == section_of(s_cursor) ? THEME_GOLD : THEME_SUB);
      for (int i = 0; i < sec->count; i++) {
        int cy = y + LABEL_H + (i / s_cols) * s_cell;
        if (cy + s_cell <= grid_top || cy >= grid_bot) continue;
        int cx = x0 + (i % s_cols) * s_cell;
        int entry = sec->first + i;
        Item it = probe_item(entry);
        int ix = cx + (s_cell - ITEM_ICON_SIZE) / 2;
        int iy = cy + (s_cell - ITEM_ICON_SIZE) / 2;
        if (game_codex_seen(entry)) gfx_draw_item(ctx, &it, ix, iy);
        else gfx_draw_item_silhouette(ctx, &it, ix, iy, GColorDarkGray);
        if (entry == s_cursor) {
          graphics_context_set_stroke_color(ctx, THEME_HI);
          graphics_context_set_stroke_width(ctx, 1);
          graphics_draw_rect(ctx, GRect(cx, cy, s_cell, s_cell));
          graphics_draw_rect(ctx, GRect(cx + 1, cy + 1, s_cell - 2, s_cell - 2));
        }
      }
    }
    y += h;
  }

  // 見出し
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, b.size.w, grid_top), 0, GCornerNone);
  snprintf(buf, sizeof(buf), "CODEX %d/%d", game_codex_seen_count(), game_codex_size());
  gfx_text(ctx, buf, GRect(0, (grid_top - TEXT_H) / 2, b.size.w, LINE_H), GTextAlignmentCenter, THEME_GOLD);
  draw_rule(ctx, grid_top - PX, b.size.w);

  // 下の欄: カーソルの物の名前と番号
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, grid_bot, b.size.w, FOOTER_H), 0, GCornerNone);
  draw_rule(ctx, grid_bot, b.size.w);
  int fx = SNAP(PBL_IF_ROUND_ELSE(b.size.w / 5, 2 * PX));
  int fw = b.size.w - fx * 2;
  GTextAlignment align = PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft);
  bool seen = game_codex_seen(s_cursor);
  Item it = probe_item(s_cursor);
  if (seen) {
    game_codex_name(s_cursor, buf, sizeof(buf));
    gfx_text(ctx, buf, GRect(fx, grid_bot + 2 * PX, fw, LINE_H), align, gfx_rarity_color(&it));
  } else {
    gfx_text(ctx, "???", GRect(fx, grid_bot + 2 * PX, fw, LINE_H), align, THEME_DIM);
  }
  snprintf(buf, sizeof(buf), "No.%03d  %s", s_cursor + 1, seen ? "SELECT: info" : "Not found yet");
  gfx_text(ctx, buf, GRect(fx, grid_bot + 2 * PX + LINE_H, fw, LINE_H), align, THEME_SUB);
}

// カーソルが見えるようにスクロールする（区切りの先頭の行では見出しも見せる）
static void keep_cursor_visible(void) {
  int view_h = layer_get_bounds(s_layer).size.h - MENU_HEADER_H - FOOTER_H;
  int y = entry_y(s_cursor);
  const Section *sec = &s_sections[section_of(s_cursor)];
  int top = (s_cursor - sec->first) < s_cols ? y - LABEL_H : y;
  if (top < s_scroll) s_scroll = top;
  if (y + s_cell > s_scroll + view_h) s_scroll = y + s_cell - view_h;
  if (s_scroll < 0) s_scroll = 0;
}

static void move_cursor(int delta) {
  int size = game_codex_size();
  s_cursor = (s_cursor + delta + size) % size;
  keep_cursor_visible();
  layer_mark_dirty(s_layer);
}

static void up_click(ClickRecognizerRef rec, void *ctx) { move_cursor(-1); }
static void down_click(ClickRecognizerRef rec, void *ctx) { move_cursor(1); }

static void select_click(ClickRecognizerRef rec, void *ctx) {
  if (!game_codex_seen(s_cursor)) {
    vibes_short_pulse();
    return;
  }
  Item it = probe_item(s_cursor);
  item_window_push_copy(&it);
}

// 長押し: 次の区切りの先頭へ（最後の区切りからは最初へ）。見出しを一番上に出す
static void select_long_click(ClickRecognizerRef rec, void *ctx) {
  int k = (section_of(s_cursor) + 1) % s_section_count;
  s_cursor = s_sections[k].first;
  s_scroll = entry_y(s_cursor) - LABEL_H;
  keep_cursor_visible();
  layer_mark_dirty(s_layer);
}

static void click_config(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 120, up_click);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 120, down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_click, NULL);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(root);
  build_sections();
  // 横に6マス（1行 = 1つの形の素材6段階）。余った幅はマスを大きくして使う
  int usable = b.size.w - SNAP(PBL_IF_ROUND_ELSE(b.size.w / 6, 2 * PX));
  s_cols = usable / ITEM_ICON_SIZE;
  if (s_cols > TIER_COUNT) s_cols = TIER_COUNT;
  if (s_cols < 1) s_cols = 1;
  s_cell = usable / s_cols;
  s_layer = layer_create(b);
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(root, s_layer);
  keep_cursor_visible();
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
  s_layer = NULL;
  window_destroy(window);
  s_window = NULL;
}

void codex_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
