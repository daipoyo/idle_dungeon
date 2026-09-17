#pragma once
#include <pebble.h>
#include "gfx.h"

// ============================================================
// 施設の人物の見出し（顔とセリフ）
//   お店・鍛冶屋・鑑定屋・保管庫・回収代行業者の画面の一番上に置く
// ============================================================
#if defined(PBL_ROUND)
#define NPC_HEADER_H SNAP(PBL_DISPLAY_HEIGHT * 34 / 100)
#else
#define NPC_HEADER_H (IS_LARGE_SCREEN ? 58 : 46)
#endif

// face_resource は RESOURCE_ID_IMG_FACE_* など。speech は描いている間残る文字列を渡す
Layer *npc_header_create(GRect frame, uint32_t face_resource, const char *speech);
void npc_header_destroy(Layer *layer);
void npc_header_say(Layer *layer, const char *speech);

// セリフの候補から1つ選ぶ（直前と同じものは避ける）
const char *npc_pick(const char *const *lines, int count, const char *previous);
#define NPC_LINES(a) (a), (int)(sizeof(a) / sizeof((a)[0]))
