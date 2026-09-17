// このファイルは tools/gen_art.py（tools/gb_items.py）で自動生成されています。直接編集しないでください。
#pragma once
#include <pebble.h>

// 図鑑アイテムの絵: 12x12 ドット、1つ 78バイト（6色 + 4bit の絵）
#define ITEM_ICON_DOTS 12
#define ITEM_ICON_RECORD 78
#define ITEM_ICON_SPECIAL_FIRST 150   // 固有・セット装備の絵はこの番号から
#define UI_ICON_FIRST 250             // 画面用のアイコンはこの番号から
#define UI_ICON_DUNGEONS 0
#define UI_ICON_SHOP 1
#define UI_ICON_STATUS 2
#define UI_ICON_STASH 3
#define UI_ICON_SMITH 4
#define UI_ICON_APPRAISER 5
#define UI_ICON_LOST_GEAR 6
#define UI_ICON_CODEX 7
#define UI_ICON_SETTINGS 8
#define UI_ICON_PORTAL 9
#define UI_ICON_WALK_BACK 10
#define UI_ICON_POTION 11
#define UI_ICON_PORTAL_SCROLL 12
#define UI_ICON_IDENTIFY_SCROLL 13

// 名前: item_names.bin は 1つ 21 バイト（20文字 + 終端）
#define ITEM_NAME_RECORD 21
#define ITEM_NAME_SHAPE_FIRST 0
#define ITEM_NAME_SPECIAL_FIRST 150
#define ITEM_NAME_SET_FIRST 250
#define ITEM_NAME_MATERIAL_FIRST 262

// 素材の色（暗・中・明）[系統][段階]
static const uint8_t ITEM_TIER_COLORS[4][6][3] = {
  { { 0xE4, 0xF8, 0xFE }, { 0xD5, 0xEA, 0xFF }, { 0xD6, 0xEB, 0xEF }, { 0xE0, 0xF4, 0xFD }, { 0xCA, 0xDF, 0xFF }, { 0xD2, 0xE7, 0xFB } },  // metal
  { { 0xE9, 0xFE, 0xFF }, { 0xE4, 0xF9, 0xFA }, { 0xE1, 0xF6, 0xFB }, { 0xE0, 0xF4, 0xFD }, { 0xCA, 0xDF, 0xFF }, { 0xD2, 0xE7, 0xFB } },  // soft
  { { 0xD4, 0xE9, 0xFE }, { 0xE4, 0xF9, 0xFE }, { 0xE0, 0xE5, 0xFA }, { 0xE0, 0xF4, 0xFD }, { 0xCA, 0xDF, 0xFF }, { 0xD2, 0xE7, 0xFB } },  // wood
  { { 0xE9, 0xFE, 0xFF }, { 0xE4, 0xF4, 0xF9 }, { 0xD5, 0xEA, 0xFF }, { 0xE0, 0xF4, 0xFD }, { 0xCA, 0xDF, 0xFF }, { 0xD2, 0xE7, 0xFB } },  // jewel
};
