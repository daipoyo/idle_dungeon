// このファイルは tools/gen_art.py で自動生成されています。直接編集しないでください。
#pragma once

#include <stdint.h>

// 文字マップの記号 -> Pebble GColor8 (argb)。0 は透明。
static const uint8_t PAL_LUT[128] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xE9, 0xC6, 0xEF, 0x00, 0xD8, 0x00, 0xC8, 0xFB, 0xEB, 0xE1, 0xC0, 0xDD, 0xF2, 0xC1, 0xF4,
  0xD1, 0xDA, 0xF0, 0xF5, 0xCA, 0xCB, 0x00, 0xFF, 0xD2, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xD4, 0xD0, 0xDF, 0xC4, 0xFA, 0xF1, 0xEA, 0xF6, 0xD6, 0xED, 0xD5, 0xFE, 0xF7, 0xC2, 0xE4,
  0xE2, 0xE6, 0xE0, 0xF9, 0xC5, 0xDB, 0xE5, 0xD7, 0xE7, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

#define HERO_IDLE_HAND_X 8
#define HERO_IDLE_HAND_Y 9
static const char *const HERO_IDLE[HERO_MAP_H] = {
  "...KKKKK....",
  "..KhhhhhK...",
  ".KhhhhhhhK..",
  ".KhhhheeeK..",
  ".KhhheKeKK..",
  ".KhhheeeeK..",
  "..KKeeeeK...",
  "..K21111K...",
  ".K2111111K..",
  ".KeK1111eK..",
  ".KK22222KK..",
  "..K2KKK2K...",
  "..K2K.K2K...",
  "..K2K.K2K...",
  ".KKKK.KKKK..",
  "............",
};

#define HERO_WALK0_HAND_X 8
#define HERO_WALK0_HAND_Y 9
static const char *const HERO_WALK0[HERO_MAP_H] = {
  "...KKKKK....",
  "..KhhhhhK...",
  ".KhhhhhhhK..",
  ".KhhhheeeK..",
  ".KhhheKeKK..",
  ".KhhheeeeK..",
  "..KKeeeeK...",
  "..K21111K...",
  ".K2111111K..",
  ".KeK1111eK..",
  ".KK22222KK..",
  "..K2KKK2K...",
  ".K2K...K2K..",
  ".K2K...K2K..",
  "KKKK...KKKK.",
  "............",
};

#define HERO_WALK1_HAND_X 8
#define HERO_WALK1_HAND_Y 9
static const char *const HERO_WALK1[HERO_MAP_H] = {
  "...KKKKK....",
  "..KhhhhhK...",
  ".KhhhhhhhK..",
  ".KhhhheeeK..",
  ".KhhheKeKK..",
  ".KhhheeeeK..",
  "..KKeeeeK...",
  "..K21111K...",
  ".K2111111K..",
  ".KeK1111eK..",
  ".KK22222KK..",
  "..K2KKK2K...",
  "..K2KK2K....",
  "...K2K2K....",
  "...KKKKKK...",
  "............",
};

#define HERO_ATTACK_HAND_X 8
#define HERO_ATTACK_HAND_Y 8
static const char *const HERO_ATTACK[HERO_MAP_H] = {
  "...KKKKK....",
  "..KhhhhhK...",
  ".KhhhhhhhK..",
  ".KhhhheeeK..",
  ".KhhheKeKK..",
  ".KhhheeeeK..",
  "..KKeeeeK...",
  "..K21111KK..",
  ".K21111eeK..",
  ".K21111KK...",
  ".KK22222K...",
  "..K2KKK2K...",
  ".K2K...K2K..",
  ".K2K...K2K..",
  "KKKK...KKKK.",
  "............",
};

#define WPN_SWORD_REST_W 8
#define WPN_SWORD_REST_H 7
#define WPN_SWORD_REST_AX 2
#define WPN_SWORD_REST_AY 5
static const char *const WPN_SWORD_REST[7] = {
  "......K.",
  ".....K5K",
  "....K54K",
  "..KK54K.",
  ".K654K..",
  "..e6K...",
  "...K....",
};

#define WPN_SWORD_SWING_W 9
#define WPN_SWORD_SWING_H 5
#define WPN_SWORD_SWING_AX 1
#define WPN_SWORD_SWING_AY 2
static const char *const WPN_SWORD_SWING[5] = {
  "..K......",
  ".K6KKKK..",
  ".e65555K.",
  ".K6KKKK..",
  "..K......",
};

#define WPN_AXE_REST_W 8
#define WPN_AXE_REST_H 7
#define WPN_AXE_REST_AX 1
#define WPN_AXE_REST_AY 5
static const char *const WPN_AXE_REST[7] = {
  "....KK..",
  "...K55K.",
  "..K5445K",
  "..K6554K",
  ".K6KK5K.",
  ".eK..K..",
  "........",
};

#define WPN_AXE_SWING_W 10
#define WPN_AXE_SWING_H 5
#define WPN_AXE_SWING_AX 1
#define WPN_AXE_SWING_AY 2
static const char *const WPN_AXE_SWING[5] = {
  "......KK..",
  "..KKKK55K.",
  ".e6666445K",
  "..KKKK55K.",
  "......KK..",
};
