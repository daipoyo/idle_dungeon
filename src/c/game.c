#include "game.h"
#include <stdlib.h>

#define SAVE_KEY_V1 1
#define SAVE_KEY 2
#define ITEM_MAX 250

// ============================================================
// アイテム・ダンジョンのカタログ（固定データ）
// ============================================================
const ItemDef g_items[ITEM_COUNT] = {
  { "Rusty Sword",   ITEM_WEAPON,    3,  0,  20 },
  { "Iron Sword",    ITEM_WEAPON,    6,  0,  60 },
  { "Steel Blade",   ITEM_WEAPON,   10,  0, 150 },
  { "Battle Axe",    ITEM_WEAPON,   14,  0, 300 },
  { "Cloth Armor",   ITEM_ARMOR,     0,  3,  20 },
  { "Leather Armor", ITEM_ARMOR,     0,  6,  60 },
  { "Chain Mail",    ITEM_ARMOR,     0, 10, 150 },
  { "Plate Armor",   ITEM_ARMOR,     0, 14, 300 },
  { "Lucky Coin",    ITEM_ACCESSORY, 1,  1,  40 },
  { "Power Ring",    ITEM_ACCESSORY, 4,  0, 120 },
  { "Guard Charm",   ITEM_ACCESSORY, 0,  4, 120 },
  { "Ancient Relic", ITEM_ACCESSORY, 6,  6, 400 },
  { "Healing Herb",  ITEM_MATERIAL,  0,  0,  10 },
  { "Monster Fang",  ITEM_MATERIAL,  0,  0,  15 },
  { "Gem Shard",     ITEM_MATERIAL,  0,  0,  50 },
  { "Dragon Scale",  ITEM_MATERIAL,  0,  0, 200 },
};

const DungeonDef g_dungeons[DUNGEON_COUNT] = {
  { "Slime Cave",      0,  20,   5,  15, { 0, 4, 12, 255 } },
  { "Goblin Woods",    8,  30,  10,  25, { 1, 5, 13, 255 } },
  { "Ancient Ruins",  18,  45,  20,  45, { 2, 6,  8,  14 } },
  { "Dragon's Lair",  30,  60,  40,  90, { 3, 7,  9,  15 } },
  { "Sunken Temple",  45,  90,  70, 150, { 10, 11, 14, 15 } },
  { "Abyssal Depths", 65, 120, 120, 250, { 3, 7,  11, 15 } },
};

// ============================================================
// セーブデータ
// ============================================================
typedef struct {
  int32_t gold;
  uint32_t run_started;          // 出発した時刻（UNIX時間）
  uint16_t runs;                 // 探索した回数
  uint16_t wins;                 // 成功した回数
  uint8_t item_count[ITEM_COUNT];
  int8_t equip[3];               // ItemType順: 武器・防具・装飾品
  uint8_t current_dungeon;
  uint8_t location;              // Location
} SaveData;

// 旧バージョン（v1.0）のセーブデータ形式。読み込み時に新形式へ移行する。
typedef struct {
  uint32_t last_active;
  int32_t gold;
  uint8_t item_count[ITEM_COUNT];
  int8_t equip_weapon;
  int8_t equip_armor;
  int8_t equip_accessory;
  uint8_t current_dungeon;
} SaveDataV1;

static SaveData s_save;
static int64_t s_run_start_ms;   // 出発時刻（ミリ秒）
static char s_log[64] = "Welcome, adventurer!";

static int64_t now_ms(void) {
  time_t t;
  uint16_t ms;
  time_ms(&t, &ms);
  return (int64_t)t * 1000 + ms;
}

void game_save(void) {
  persist_write_data(SAVE_KEY, &s_save, sizeof(SaveData));
}

static void reset_save(void) {
  memset(&s_save, 0, sizeof(SaveData));
  s_save.gold = 30;
  s_save.equip[0] = s_save.equip[1] = s_save.equip[2] = NO_ITEM;
  s_save.location = LOC_TOWN;
}

static void load_game(void) {
  reset_save();
  if (persist_exists(SAVE_KEY)) {
    persist_read_data(SAVE_KEY, &s_save, sizeof(SaveData));
  } else if (persist_exists(SAVE_KEY_V1)) {
    SaveDataV1 old;
    memset(&old, 0, sizeof(old));
    persist_read_data(SAVE_KEY_V1, &old, sizeof(old));
    s_save.gold = old.gold;
    memcpy(s_save.item_count, old.item_count, ITEM_COUNT);
    s_save.equip[ITEM_WEAPON] = old.equip_weapon;
    s_save.equip[ITEM_ARMOR] = old.equip_armor;
    s_save.equip[ITEM_ACCESSORY] = old.equip_accessory;
    s_save.current_dungeon = old.current_dungeon;
    persist_delete(SAVE_KEY_V1);
    game_save();
  }
  // 壊れたデータへの保険
  if (s_save.current_dungeon >= DUNGEON_COUNT) s_save.current_dungeon = 0;
  if (s_save.location > LOC_DUNGEON) s_save.location = LOC_TOWN;
  for (int i = 0; i < 3; i++) {
    if (s_save.equip[i] < NO_ITEM || s_save.equip[i] >= ITEM_COUNT) s_save.equip[i] = NO_ITEM;
  }
}

// ============================================================
// ステータス
// ============================================================
int game_atk(void) {
  int v = 0;
  for (int i = 0; i < 3; i++) {
    if (s_save.equip[i] >= 0) v += g_items[s_save.equip[i]].atk;
  }
  return v;
}

int game_def(void) {
  int v = 0;
  for (int i = 0; i < 3; i++) {
    if (s_save.equip[i] >= 0) v += g_items[s_save.equip[i]].def;
  }
  return v;
}

int game_power(void) { return BASE_POWER + game_atk() + game_def(); }
int32_t game_gold(void) { return s_save.gold; }
int game_runs(void) { return s_save.runs; }
int game_wins(void) { return s_save.wins; }

// ============================================================
// 探索
// ============================================================
Location game_location(void) { return (Location)s_save.location; }
int game_current_dungeon(void) { return s_save.current_dungeon; }
int game_clear_ms(void) { return g_dungeons[s_save.current_dungeon].clear_time_sec * 1000; }

int game_progress_ms(void) {
  if (s_save.location != LOC_DUNGEON) return 0;
  int64_t p = now_ms() - s_run_start_ms;
  if (p < 0) p = 0;
  if (p > game_clear_ms()) p = game_clear_ms();
  return (int)p;
}

bool game_dungeon_unlocked(int idx) {
  return idx == 0 || game_power() >= g_dungeons[idx].required_power;
}

// 1回の探索結果を判定する（成否・獲得ゴールド・獲得アイテム）
static void simulate_one_run(RunResult *r) {
  const DungeonDef *d = &g_dungeons[s_save.current_dungeon];
  int diff = game_power() - d->required_power;
  int chance = 50 + diff * 5;
  if (chance < 10) chance = 10;
  if (chance > 95) chance = 95;

  r->dungeon = s_save.current_dungeon;
  r->success = (rand() % 100) < chance;
  int span = (d->gold_max - d->gold_min) + 1;
  r->gold = d->gold_min + (span > 0 ? (rand() % span) : 0);
  r->item_id = NO_ITEM;

  if (r->success) {
    if ((rand() % 100) < 60) {
      uint8_t candidates[4];
      int n = 0;
      for (int i = 0; i < 4; i++) {
        if (d->loot_ids[i] != 255) candidates[n++] = d->loot_ids[i];
      }
      if (n > 0) r->item_id = candidates[rand() % n];
    }
  } else {
    r->gold = r->gold / 4;
  }
}

// 探索を終えて町へ戻る
static void finish_run(RunResult *r) {
  simulate_one_run(r);
  s_save.gold += r->gold;
  if (r->item_id >= 0 && s_save.item_count[r->item_id] < ITEM_MAX) {
    s_save.item_count[r->item_id]++;
  }
  if (s_save.runs < 65535) s_save.runs++;
  if (r->success && s_save.wins < 65535) s_save.wins++;
  s_save.location = LOC_TOWN;

  if (!r->success) {
    snprintf(s_log, sizeof(s_log), "Retreated... +%dG", r->gold);
  } else if (r->item_id >= 0) {
    snprintf(s_log, sizeof(s_log), "Got %s! +%dG", g_items[r->item_id].name, r->gold);
  } else {
    snprintf(s_log, sizeof(s_log), "Cleared! +%dG", r->gold);
  }
  game_save();
}

bool game_update(RunResult *out) {
  if (s_save.location != LOC_DUNGEON) return false;
  int64_t now = now_ms();
  if (now < s_run_start_ms) s_run_start_ms = now; // 時計が戻った場合
  if (now - s_run_start_ms < game_clear_ms()) return false;
  finish_run(out);
  return true;
}

bool game_depart(int idx) {
  if (idx < 0 || idx >= DUNGEON_COUNT) return false;
  if (s_save.location != LOC_TOWN) return false;
  if (!game_dungeon_unlocked(idx)) {
    snprintf(s_log, sizeof(s_log), "Too dangerous! Need POW %d", g_dungeons[idx].required_power);
    return false;
  }
  s_save.current_dungeon = (uint8_t)idx;
  s_save.location = LOC_DUNGEON;
  s_run_start_ms = now_ms();
  s_save.run_started = (uint32_t)(s_run_start_ms / 1000);
  snprintf(s_log, sizeof(s_log), "Exploring...");
  game_save();
  return true;
}

void game_retreat(void) {
  if (s_save.location != LOC_DUNGEON) return;
  s_save.location = LOC_TOWN;
  snprintf(s_log, sizeof(s_log), "Back to town, no loot.");
  game_save();
}

// ============================================================
// 装備・所持品
// ============================================================
int game_item_count(int id) { return s_save.item_count[id]; }
int game_equipped(ItemType slot) { return s_save.equip[slot]; }

bool game_is_equipped(int id) {
  return s_save.equip[0] == id || s_save.equip[1] == id || s_save.equip[2] == id;
}

bool game_can_change_gear(void) { return s_save.location == LOC_TOWN; }

void game_unequip(ItemType slot) {
  if (slot > ITEM_ACCESSORY || !game_can_change_gear()) return;
  s_save.equip[slot] = NO_ITEM;
  game_save();
}

void game_toggle_equip(int id) {
  ItemType t = g_items[id].type;
  if (t == ITEM_MATERIAL || s_save.item_count[id] == 0 || !game_can_change_gear()) return;
  s_save.equip[t] = (s_save.equip[t] == id) ? NO_ITEM : id;
  game_save();
}

int game_owned_kinds(void) {
  int n = 0;
  for (int i = 0; i < ITEM_COUNT; i++) {
    if (s_save.item_count[i] > 0) n++;
  }
  return n;
}

int game_owned_nth(int n) {
  for (int i = 0; i < ITEM_COUNT; i++) {
    if (s_save.item_count[i] > 0 && n-- == 0) return i;
  }
  return NO_ITEM;
}

// ============================================================
// お店
// ============================================================
BuyResult game_buy(int id) {
  if (s_save.gold < g_items[id].price) return BUY_NO_GOLD;
  if (s_save.item_count[id] >= ITEM_MAX) return BUY_FULL;
  s_save.gold -= g_items[id].price;
  s_save.item_count[id]++;
  game_save();
  return BUY_OK;
}

int game_sell_price(int id) { return g_items[id].price / 2; }

SellResult game_sell(int id) {
  if (s_save.item_count[id] == 0) return SELL_NONE;
  if (game_is_equipped(id) && s_save.item_count[id] <= 1) return SELL_EQUIPPED;
  s_save.item_count[id]--;
  s_save.gold += game_sell_price(id);
  game_save();
  return SELL_OK;
}

// ============================================================
// メッセージ
// ============================================================
const char *game_log(void) { return s_log; }

// ============================================================
// 初期化（放置中に探索が終わっていたらここで精算する）
// ============================================================
void game_init(void) {
  srand((unsigned int)time(NULL));
  load_game();
  if (s_save.location == LOC_DUNGEON) {
    s_run_start_ms = (int64_t)s_save.run_started * 1000;
    RunResult r;
    game_update(&r);   // 終わっていれば町に戻り、メッセージも設定される
    if (s_save.location == LOC_DUNGEON) {
      snprintf(s_log, sizeof(s_log), "Exploring...");
    }
  }
}
