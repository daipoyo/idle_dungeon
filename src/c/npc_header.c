#include "npc_header.h"
#include <stdlib.h>

typedef struct {
  GBitmap *face;
  const char *speech;
} NpcHeader;

static void update_proc(Layer *layer, GContext *ctx) {
  NpcHeader *h = layer_get_data(layer);
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  GSize face = h->face ? gbitmap_get_bounds(h->face).size : GSize(0, 0);
#if defined(PBL_ROUND)
  // 丸型: 顔を左寄りに置き、セリフは右側に下寄せ（上端は狭いので避ける）
  int face_x = SNAP(b.size.w / 6);
  int x = face_x + face.w + 2 * PX;
  int ty = SNAP(b.size.h / 3);
  GRect text = GRect(x, ty, b.size.w - x - face_x, b.size.h - ty - PX);
#else
  int face_x = 2 * PX;
  int x = face_x + face.w + 3 * PX;
  GRect text = GRect(x, 2 * PX, b.size.w - x - 2 * PX, b.size.h - 3 * PX);
#endif
  if (h->face) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, h->face, GRect(face_x, b.size.h - face.h - 2 * PX, face.w, face.h));
  }
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, b.size.h - PX, b.size.w, PX), 0, GCornerNone);
  if (h->speech) gfx_text(ctx, h->speech, text, GTextAlignmentLeft, THEME_FG);
}

Layer *npc_header_create(GRect frame, uint32_t face_resource, const char *speech) {
  Layer *layer = layer_create_with_data(frame, sizeof(NpcHeader));
  NpcHeader *h = layer_get_data(layer);
  h->face = gbitmap_create_with_resource(face_resource);
  h->speech = speech;
  layer_set_update_proc(layer, update_proc);
  return layer;
}

void npc_header_destroy(Layer *layer) {
  if (!layer) return;
  NpcHeader *h = layer_get_data(layer);
  if (h->face) gbitmap_destroy(h->face);
  layer_destroy(layer);
}

void npc_header_say(Layer *layer, const char *speech) {
  if (!layer) return;
  NpcHeader *h = layer_get_data(layer);
  h->speech = speech;
  layer_mark_dirty(layer);
}

const char *npc_pick(const char *const *lines, int count, const char *previous) {
  if (count <= 0) return NULL;
  int i = rand() % count;
  // 直前と同じなら、ほかのどれかにずらす
  if (count > 1 && lines[i] == previous) i = (i + 1 + rand() % (count - 1)) % count;
  return lines[i];
}
