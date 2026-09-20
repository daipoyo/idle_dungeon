#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "qr_data.h"

// ============================================================
// 作者への支援（酒場から開く）
//   スマホのカメラで読めるように QR を大きく描き、下に URL を文字でも出す。
//   反射液晶は暗いと読み取りづらいので、開いている間はバックライトを点けておく
// ============================================================
static Window *s_window;
static Layer *s_layer;

static void update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  // 下に URL の1行を置き、残りの高さいっぱいに QR を入れる
  int text_h = LINE_H + 2 * PX;
  int room = (b.size.h - text_h) < b.size.w ? (b.size.h - text_h) : b.size.w;
  int scale = room / QR_SUPPORT_SIZE;
  if (scale < 1) scale = 1;
  int size = scale * QR_SUPPORT_SIZE;
  int qx = (b.size.w - size) / 2;
  int qy = (b.size.h - text_h - size) / 2;
  if (qy < 0) qy = 0;
  gfx_draw_charmap(ctx, QR_SUPPORT, QR_SUPPORT_SIZE, QR_SUPPORT_SIZE, qx, qy, scale, false, NULL);

  gfx_text(ctx, QR_SUPPORT_URL, GRect(0, qy + size + PX, b.size.w, LINE_H), GTextAlignmentCenter,
           GColorBlack);
}

static void window_appear(Window *window) {
  light_enable(true);   // 読み取りやすいように明るくしておく
}

static void window_disappear(Window *window) {
  light_enable(false);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_layer, update_proc);
  layer_add_child(root, s_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_layer);
  s_layer = NULL;
  window_destroy(window);
  s_window = NULL;
}

void support_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
    .disappear = window_disappear,
  });
  window_stack_push(s_window, true);
}
