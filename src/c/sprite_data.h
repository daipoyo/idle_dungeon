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

#define WPN_DAGGER_REST_W 6
#define WPN_DAGGER_REST_H 6
#define WPN_DAGGER_REST_AX 1
#define WPN_DAGGER_REST_AY 4
static const char *const WPN_DAGGER_REST[6] = {
  "....K.",
  "...K5K",
  "..K54K",
  ".K64K.",
  ".e6K..",
  "..K...",
};

#define WPN_MACE_REST_W 8
#define WPN_MACE_REST_H 8
#define WPN_MACE_REST_AX 1
#define WPN_MACE_REST_AY 6
static const char *const WPN_MACE_REST[8] = {
  "....KK..",
  "...K55K.",
  "..K5445K",
  "..K5445K",
  "..K655K.",
  ".K6KKK..",
  ".eK.....",
  "........",
};

#define WPN_HAMMER_REST_W 10
#define WPN_HAMMER_REST_H 9
#define WPN_HAMMER_REST_AX 2
#define WPN_HAMMER_REST_AY 7
static const char *const WPN_HAMMER_REST[9] = {
  "...KKKKKK.",
  "..K555555K",
  "..K544444K",
  "..K444444K",
  "...KK6KKK.",
  "...K6K....",
  "..K6K.....",
  "..eK......",
  "..........",
};

#define WPN_POLE_REST_W 10
#define WPN_POLE_REST_H 10
#define WPN_POLE_REST_AX 2
#define WPN_POLE_REST_AY 7
static const char *const WPN_POLE_REST[10] = {
  "........K.",
  ".......K5K",
  "......K54K",
  ".....K64K.",
  "....K6KK..",
  "...K6K....",
  "..K6K.....",
  ".KeK......",
  "K6K.......",
  ".K........",
};

#define WPN_STAFF_REST_W 10
#define WPN_STAFF_REST_H 10
#define WPN_STAFF_REST_AX 2
#define WPN_STAFF_REST_AY 7
static const char *const WPN_STAFF_REST[10] = {
  ".......KK.",
  "......K55K",
  "......K54K",
  ".....K6KK.",
  "....K6K...",
  "...K6K....",
  "..K6K.....",
  ".KeK......",
  "K6K.......",
  ".K........",
};

#define WPN_FIST_REST_W 7
#define WPN_FIST_REST_H 6
#define WPN_FIST_REST_AX 1
#define WPN_FIST_REST_AY 4
static const char *const WPN_FIST_REST[6] = {
  ".K.K.K.",
  "K5K5K5K",
  "K4K4K4K",
  "K44444K",
  ".e444K.",
  "..KKK..",
};

static const char *const GEAR_HEAD_CLOTH[HERO_MAP_H] = {
  "..KKKKKK....",
  ".K778888K...",
  "K78888888K..",
  "K788K.......",
  "K788K.......",
  "K789K.......",
  ".K99K.......",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
};

static const char *const GEAR_HEAD_HELM[HERO_MAP_H] = {
  "..KKKKKK....",
  ".K778888K...",
  "K78888888K..",
  "K788999999K.",
  "K78K........",
  "K78K........",
  ".KK.........",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
};

static const char *const GEAR_HEAD_CROWN[HERO_MAP_H] = {
  "..7.7.7.7...",
  "..8j8j8j8...",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
};

static const char *const GEAR_OFF_SHIELD[HERO_MAP_H] = {
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "KKKKK.......",
  "K8jj9K......",
  "K8jj9K......",
  "K8j7j9K.....",
  "K8jj9K......",
  ".K8j9K......",
  "..K99K......",
  "...KK.......",
  "............",
  "............",
};

static const char *const GEAR_OFF_FOCUS[HERO_MAP_H] = {
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  "............",
  ".K..........",
  "K7K.........",
  "789K........",
  "K9K.........",
  ".K..........",
  "............",
  "............",
  "............",
  "............",
};

#define ENEMY_MAP_COUNT 8
#define ENEMY_MAP_SIZE 12
static const char *const ENEMY_0[ENEMY_MAP_SIZE] = {
  "............",
  "............",
  "............",
  ".....KK.....",
  "....KGLK....",
  "...KGGGGK...",
  "..KGLGGGGK..",
  ".KGKLGGKGGK.",
  ".KGKLGGKGGK.",
  "KGGGGGGGGGdK",
  "KdGGGGGGGddK",
  ".KKKKKKKKKK.",
};

static const char *const ENEMY_1[ENEMY_MAP_SIZE] = {
  "............",
  "K..........K",
  "KEK.KKKK.KEK",
  ".KEKEEEEKEK.",
  "..KEEEEEEK..",
  "..KYKEYEEK..",
  "..KEEEEEEK..",
  "...KYKYEK...",
  "..KoooooK...",
  ".KEKoooKE...",
  "..KoKoKoK...",
  "..KK...KK...",
};

static const char *const ENEMY_2[ENEMY_MAP_SIZE] = {
  "....KKKK....",
  "...KWWWWK...",
  "..KWWWWWWg..",
  "..WKKWKKWg..",
  "..WWWWWWWg..",
  "...WKWKWK...",
  "....KWWK....",
  "..W.WWWW.W..",
  ".KW..WW..WK.",
  "....WWWW....",
  "....W..W....",
  "...WW..WW...",
};

static const char *const ENEMY_3[ENEMY_MAP_SIZE] = {
  "............",
  ".......K....",
  "..KK..KrK...",
  ".KRRK.KrrK..",
  "KYRRRKKrrrK.",
  "KRRRRRRKKK..",
  ".KKRRRRRRRK.",
  "..KYYYRRRRRK",
  "..KYYYRRRRK.",
  "...KYYRRRK..",
  "...KRKKRK...",
  "...KK.KK....",
};

static const char *const ENEMY_4[ENEMY_MAP_SIZE] = {
  "............",
  "....KKKK....",
  "...KCuuuK...",
  "..KCuuuuuK..",
  "..Cuuuuuuu..",
  "..uKuuKuuu..",
  "..uuuuuuuu..",
  "..BBBBBBBB..",
  "..u.u..u.u..",
  "...u.u..u...",
  "..u.u..u.u..",
  "............",
};

static const char *const ENEMY_5[ENEMY_MAP_SIZE] = {
  "............",
  "..p......p..",
  "...p.KK.p...",
  "....KWWK....",
  "...KWWWWK...",
  "..KWWRRWWK..",
  "..KWRKKRWK..",
  "..KWWRRWWK..",
  "...KWWWWK...",
  "....KppK....",
  "...p.pp.p...",
  "..p..p...p..",
};

static const char *const ENEMY_6[ENEMY_MAP_SIZE] = {
  "............",
  "...KKKKKK...",
  "..KRRWRRRK..",
  ".KRRRRWRRRK.",
  "KRRRWRRRRRWK",
  "KrrrrrrrrrrK",
  "....KWWWK...",
  "....KWWWK...",
  "....KWWWK...",
  "...KWWWWWK..",
  "..KWWWWWWWK.",
  "..KKKKKKKKK.",
};

static const char *const ENEMY_7[ENEMY_MAP_SIZE] = {
  "............",
  "...BBBBBB...",
  "..BWWWWWWB..",
  ".BWWKWWKWWB.",
  ".BWWWWWWWWB.",
  ".BWWBBBBWWB.",
  "BBWWWWWWWWBB",
  "BWBWWWWWWBWB",
  "BWBWWWWWWBWB",
  "BBWWWWWWWWBB",
  "..BWWB.BWWB.",
  "..BBBB.BBBB.",
};

static const char *const *const ENEMY_MAPS[ENEMY_MAP_COUNT] = {
  ENEMY_0, ENEMY_1, ENEMY_2, ENEMY_3, ENEMY_4, ENEMY_5, ENEMY_6, ENEMY_7,
};
