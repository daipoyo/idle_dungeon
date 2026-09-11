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

#define HERO_IDLE_HAND_X 12
#define HERO_IDLE_HAND_Y 15
static const char *const HERO_IDLE[HERO_H] = {
  ".....KKKKK......",
  "...KKhhhhhKK....",
  "..KhhhhhhhhhK...",
  ".KhhhhhhhhhhhK..",
  ".KhhhhhhhhhhhhK.",
  "KhhhrrrrrrrrrK..",
  "KhHhrreeeeeeeK..",
  "KhHhheeeeeKeeK..",
  "KhHhheeeeeKeeK..",
  ".KhhheeeeeeeevK.",
  "..KhhveeeeeveK..",
  "...KhhvveeeeK...",
  "..Kcc311113eK...",
  ".Kccc3111112eK..",
  ".Kccc1111112eK..",
  ".Kcc21111112eK..",
  ".KccKyyyyyyKK...",
  ".KccK2221222K...",
  "..KKKppKKppK....",
  "....KppKKppK....",
  "....KppKKppK....",
  "....KBBKKBBK....",
  "...KBBBKBBBBK...",
  "....KKK.KKKK....",
};

#define HERO_WALK0_HAND_X 12
#define HERO_WALK0_HAND_Y 15
static const char *const HERO_WALK0[HERO_H] = {
  ".....KKKKK......",
  "...KKhhhhhKK....",
  "..KhhhhhhhhhK...",
  ".KhhhhhhhhhhhK..",
  ".KhhhhhhhhhhhhK.",
  "KhhhrrrrrrrrrK..",
  "KhHhrreeeeeeeK..",
  "KhHhheeeeeKeeK..",
  "KhHhheeeeeKeeK..",
  ".KhhheeeeeeeevK.",
  "..KhhveeeeeveK..",
  "...KhhvveeeeK...",
  "..Kcc311113eK...",
  ".Kccc3111112eK..",
  ".Kccc1111112eK..",
  ".Kcc21111112eK..",
  ".KccKyyyyyyKK...",
  ".KccK2221222K...",
  "..KKKppKKppK....",
  "...KppK..KppK...",
  "..KppK....KppK..",
  "..KBBK....KBBK..",
  ".KBBBK....KBBBK.",
  "..KKK......KKK..",
};

#define HERO_WALK1_HAND_X 12
#define HERO_WALK1_HAND_Y 15
static const char *const HERO_WALK1[HERO_H] = {
  ".....KKKKK......",
  "...KKhhhhhKK....",
  "..KhhhhhhhhhK...",
  ".KhhhhhhhhhhhK..",
  ".KhhhhhhhhhhhhK.",
  "KhhhrrrrrrrrrK..",
  "KhHhrreeeeeeeK..",
  "KhHhheeeeeKeeK..",
  "KhHhheeeeeKeeK..",
  ".KhhheeeeeeeevK.",
  "..KhhveeeeeveK..",
  "...KhhvveeeeK...",
  "..Kcc311113eK...",
  ".Kccc3111112eK..",
  ".Kccc1111112eK..",
  ".Kcc21111112eK..",
  ".KccKyyyyyyKK...",
  ".KccK2221222K...",
  "..KK.KppppKK....",
  ".....KppKppK....",
  "....KppKKppK....",
  "....KBBKKBBKK...",
  "...KBBBKKBBBBK..",
  "....KKK..KKKK...",
};

#define HERO_ATTACK_HAND_X 13
#define HERO_ATTACK_HAND_Y 12
static const char *const HERO_ATTACK[HERO_H] = {
  ".....KKKKK......",
  "...KKhhhhhKK....",
  "..KhhhhhhhhhK...",
  ".KhhhhhhhhhhhK..",
  ".KhhhhhhhhhhhhK.",
  "KhhhrrrrrrrrrK..",
  "KhHhrreeeeeeeK..",
  "KhHhheeeeeKeeK..",
  "KhHhheeeeeKeeK..",
  ".KhhheeeeeeeevK.",
  "..KhhveeeeeveK..",
  "...KhhvveeeeKK..",
  "..Kcc31111eeeeK.",
  ".Kccc311111KKK..",
  ".Kccc1111112K...",
  ".Kcc21111112K...",
  ".KccKyyyyyyK....",
  ".KccK2221222K...",
  "..KKKppKKppK....",
  "...KppK..KppK...",
  "..KppK....KppK..",
  "..KBBK....KBBK..",
  ".KBBBK....KBBBK.",
  "..KKK......KKK..",
};

#define WPN_SWORD_REST_W 10
#define WPN_SWORD_REST_H 10
#define WPN_SWORD_REST_AX 1
#define WPN_SWORD_REST_AY 2
static const char *const WPN_SWORD_REST[10] = {
  "..K.......",
  ".KyK......",
  "Key5K.....",
  ".Ky45K....",
  "..KK45K...",
  "....K45K..",
  ".....K45K.",
  "......K45K",
  ".......K4K",
  "........K.",
};

#define WPN_SWORD_SWING_W 14
#define WPN_SWORD_SWING_H 6
#define WPN_SWORD_SWING_AX 1
#define WPN_SWORD_SWING_AY 2
static const char *const WPN_SWORD_SWING[6] = {
  "...K..........",
  ".KKyKKKKKKKKK.",
  "Keeyy55555555K",
  ".KKy444444444K",
  "..KyKKKKKKKKK.",
  "...K..........",
};

#define WPN_AXE_REST_W 11
#define WPN_AXE_REST_H 9
#define WPN_AXE_REST_AX 1
#define WPN_AXE_REST_AY 1
static const char *const WPN_AXE_REST[9] = {
  ".KK........",
  "Ke6K.......",
  ".K66K.KKK..",
  "..K66K555K.",
  "...K66455K.",
  "....K6445K.",
  "...K55445K.",
  "....K5555K.",
  ".....KKKK..",
};

#define WPN_AXE_SWING_W 15
#define WPN_AXE_SWING_H 7
#define WPN_AXE_SWING_AX 1
#define WPN_AXE_SWING_AY 3
static const char *const WPN_AXE_SWING[7] = {
  "...........KKK.",
  "..........K555K",
  ".KKKKKKKKKK545K",
  "Kee66666666445K",
  ".KKKKKKKKKK545K",
  "..........K555K",
  "...........KKK.",
};
