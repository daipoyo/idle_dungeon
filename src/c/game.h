#pragma once
#include <pebble.h>

// ============================================================
// ゲームデータとロジック（画面から独立した部分）
// ============================================================
#define ITEM_COUNT 16
#define DUNGEON_COUNT 6
#define BASE_POWER 5
#define NO_ITEM (-1)

typedef enum {
  ITEM_WEAPON = 0,
  ITEM_ARMOR = 1,
  ITEM_ACCESSORY = 2,
  ITEM_MATERIAL = 3,
} ItemType;

typedef struct {
  const char *name;
  ItemType type;
  int16_t atk;
  int16_t def;
  int16_t price;
} ItemDef;

typedef struct {
  const char *name;
  int16_t required_power;
  int16_t clear_time_sec;
  int16_t gold_min;
  int16_t gold_max;
  uint8_t loot_ids[4]; // 255 = 空
} DungeonDef;

typedef enum {
  LOC_TOWN = 0,     // 町で待機中
  LOC_DUNGEON = 1,  // ダンジョン探索中
} Location;

typedef struct {
  bool success;
  int gold;
  int item_id;      // NO_ITEM = なし
  uint8_t dungeon;
} RunResult;

typedef enum { BUY_OK, BUY_NO_GOLD, BUY_FULL } BuyResult;
typedef enum { SELL_OK, SELL_NONE, SELL_EQUIPPED } SellResult;

extern const ItemDef g_items[ITEM_COUNT];
extern const DungeonDef g_dungeons[DUNGEON_COUNT];

void game_init(void);
void game_save(void);

// ---- 冒険の状態 ----
Location game_location(void);
int game_current_dungeon(void);
int game_progress_ms(void);
int game_clear_ms(void);
// 探索を進める。1回の探索が終わって町に戻ったら true を返し、結果を out に入れる。
bool game_update(RunResult *out);
bool game_dungeon_unlocked(int idx);
bool game_depart(int idx);
void game_retreat(void);

// ---- 勇者のステータス ----
int game_atk(void);
int game_def(void);
int game_power(void);
int32_t game_gold(void);
int game_runs(void);
int game_wins(void);

// ---- 装備・所持品 ----
int game_item_count(int id);
int game_equipped(ItemType slot);
bool game_is_equipped(int id);
bool game_can_change_gear(void);
void game_toggle_equip(int id);
void game_unequip(ItemType slot);
int game_owned_kinds(void);
int game_owned_nth(int n);   // 所持しているアイテムの n 番目の ID

// ---- お店 ----
BuyResult game_buy(int id);
SellResult game_sell(int id);
int game_sell_price(int id);

// ---- メッセージ（町・ダンジョン画面の下部に出す一言） ----
const char *game_log(void);
