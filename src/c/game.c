#include "game.h"
#include "steps.h"

// ============================================================
// 保存キー
//   Pebble の保存領域は 1キー 256バイトまで、アプリ全体で約4KB。
//   旧バージョンのキー（1, 2）は起動時に消す（セーブはリセット）。
// ============================================================
#define KEY_OLD_V1 1
#define KEY_OLD_V2 2
#define KEY_HERO 100
#define KEY_RUN 101
#define KEY_EQUIP 102
#define KEY_BAG 103
#define KEY_LOG 104
#define KEY_DROP 105
#define KEY_DROP_EQUIP 106
#define KEY_DROP_BAG 107
#define SAVE_VERSION 1

// ============================================================
// 固定データ
// ============================================================
const DungeonDef g_dungeons[DUNGEON_COUNT] = {
  { "Mossy Cellar",   5, 300,  1,  5, 0, { "Giant Rat", "Cave Bat", "Slime", "Rat King" } },
  { "Goblin Warren",  6, 350,  5, 10, 1, { "Goblin", "Goblin Archer", "Hobgoblin", "Goblin Chief" } },
  { "Sunken Crypt",   8, 400, 10, 16, 2, { "Skeleton", "Ghoul", "Wraith", "Bone Lord" } },
  { "Fungal Caverns", 9, 450, 16, 22, 0, { "Myconid", "Spore Bat", "Mold Beast", "Spore Mother" } },
  { "Drowned Temple", 10, 500, 22, 28, 4, { "Merfolk", "Sea Snake", "Drowned One", "Tide Priestess" } },
  { "Ember Forge",    12, 600, 28, 35, 3, { "Fire Imp", "Magma Hound", "Golem", "Iron Colossus" } },
  { "Frost Spire",    13, 700, 35, 42, 4, { "Ice Wolf", "Frost Troll", "Yeti", "Frost Wyrm" } },
  { "Abyssal Gate",   15, 800, 42, 50, 5, { "Imp Lord", "Hellhound", "Pit Fiend", "Demon Lord" } },
};

// フェーズ1の仮の基本アイテム（フェーズ2・3で約1000種類に増やす）
const BaseDef g_bases[] = {
  { "Short Sword",   SLOT_WEAPON,  1 },
  { "Buckler",       SLOT_OFFHAND, 10 },
  { "Leather Cap",   SLOT_HEAD,    7 },
  { "Quilted Armor", SLOT_BODY,    5 },
  { "Gloves",        SLOT_HANDS,   6 },
  { "Boots",         SLOT_FEET,    4 },
  { "Amulet",        SLOT_AMULET,  11 },
  { "Ring",          SLOT_RING1,   9 },
};
const int g_base_count = sizeof(g_bases) / sizeof(g_bases[0]);

const char *const g_slot_names[EQUIP_SLOTS] = {
  "Weapon", "Shield", "Head", "Body", "Hands", "Feet", "Amulet", "Ring", "Ring",
};

// ============================================================
// 保存データ
// ============================================================
typedef struct {
  uint8_t version;
  uint8_t level;
  uint8_t potions;
  uint8_t portals;
  int32_t gold;
  int32_t xp;
  uint8_t cleared;          // ボスを倒したダンジョン（ビット）
  uint8_t auto_return_pct;  // 0 = しない
  uint8_t vibrate;
  uint8_t pad;
  uint16_t runs;
  uint16_t deaths;
} Hero;

typedef struct {
  uint8_t mode;             // RunMode
  uint8_t dungeon;
  uint8_t boss_done;
  uint8_t drop_checked;     // この探索で残した物の場所を通ったか
  int32_t hp;
  uint32_t depth;           // 入口からの歩数（奥へ進んだ分）
  uint32_t return_left;     // 帰り道の残り歩数
  uint32_t next_event;      // 次の出来事が起きる位置
  uint32_t rng;
  StepSnapshot snap;
  int32_t run_gold;         // この探索で拾ったゴールド
} Run;

typedef struct {
  uint8_t active;
  uint8_t dungeon;
  uint16_t pad;
  uint32_t depth;           // 入口からの歩数
  uint32_t time;            // 死んだ時刻
  int32_t gold;
} Drop;

// ログは古い順に entries[0] 〜 entries[count-1] に並ぶ
typedef struct {
  uint8_t layout;           // LOG_LAYOUT_LINEAR（旧形式は輪になったバッファで、読み込み時に並べ直す）
  uint8_t count;
  uint16_t pad;
  LogEntry entries[LOG_SIZE];
} LogBuf;
#define LOG_LAYOUT_LINEAR 1

static Hero s_hero;
static Run s_run;
static Item s_equip[EQUIP_SLOTS];
static Item s_bag[BAG_SIZE];
static LogBuf s_log;
static Drop s_drop;
static Item s_drop_equip[EQUIP_SLOTS];
static Item s_drop_bag[BAG_SIZE];
static bool s_dirty;
static bool s_alert;            // 振動で知らせたい出来事があった
static bool s_warned_no_steps;

// ============================================================
// 乱数（探索ごとの状態を保存しておく）
// ============================================================
static uint32_t rng_next(void) {
  uint32_t x = s_run.rng ? s_run.rng : 0x9E3779B9;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  s_run.rng = x;
  return x;
}

static int rnd(int n) {
  return n > 0 ? (int)(rng_next() % (uint32_t)n) : 0;
}

// ============================================================
// ログ
// ============================================================
static uint16_t clamp16(int v) {
  return (uint16_t)(v < 0 ? 0 : (v > 65535 ? 65535 : v));
}

// 重要な出来事（普通の出来事に押し出されない）
static bool log_type_important(uint8_t type) {
  switch ((LogType)type) {
    case LOG_DEPART:
    case LOG_BOSS:
    case LOG_LEVEL:
    case LOG_DEATH:
    case LOG_RECOVER:
    case LOG_DROP_GONE:
    case LOG_AGENT:
    case LOG_SUMMARY:
      return true;
    default:
      return false;
  }
}

// 1回の更新でまとめて起きた出来事の集計（まとめのログ用）
static struct {
  int events;
  int battles;
  int gold;
  int items;
  int levels;
} s_batch;

static void log_push(LogType type, int a, int b, int c, int d) {
  if (s_log.count >= LOG_SIZE) {
    // いっぱいなら、一番古い普通の出来事を消す。
    // 重要な出来事が LOG_IMPORTANT_MAX を超えたときだけ、一番古い重要な出来事を消す。
    int important = 0, oldest_normal = -1, oldest_important = -1;
    for (int i = 0; i < s_log.count; i++) {
      if (log_type_important(s_log.entries[i].type)) {
        important++;
        if (oldest_important < 0) oldest_important = i;
      } else if (oldest_normal < 0) {
        oldest_normal = i;
      }
    }
    int victim = 0;
    if (log_type_important((uint8_t)type) && important >= LOG_IMPORTANT_MAX) victim = oldest_important;
    else if (oldest_normal >= 0) victim = oldest_normal;
    memmove(&s_log.entries[victim], &s_log.entries[victim + 1],
            sizeof(LogEntry) * (s_log.count - victim - 1));
    s_log.count--;
  }
  LogEntry *e = &s_log.entries[s_log.count++];
  e->type = (uint8_t)type;
  e->a = (uint8_t)a;
  e->b = clamp16(b);
  e->c = clamp16(c);
  e->d = clamp16(d);
  if (type != LOG_SUMMARY) s_batch.events++;
  s_dirty = true;
}

int game_log_count(void) { return s_log.count; }

// 新しい順に i 番目
static const LogEntry *log_at(int i) {
  if (i < 0 || i >= s_log.count) return NULL;
  return &s_log.entries[s_log.count - 1 - i];
}

LogStyle game_log_style(int i) {
  const LogEntry *e = log_at(i);
  if (!e) return LOG_STYLE_TOWN;
  switch ((LogType)e->type) {
    case LOG_SUMMARY:
      return LOG_STYLE_SUMMARY;
    case LOG_DEATH:
    case LOG_DROP_GONE:
    case LOG_LOW_HP:
    case LOG_NO_STEPS:
      return LOG_STYLE_DANGER;
    case LOG_BOSS:
    case LOG_LEVEL:
    case LOG_RECOVER:
    case LOG_AGENT:
      return LOG_STYLE_GREAT;
    case LOG_WELCOME:
    case LOG_TOWN:
      return LOG_STYLE_TOWN;
    default:
      return LOG_STYLE_DUNGEON;
  }
}

static const char *monster_name(int a) {
  return g_dungeons[(a / 4) % DUNGEON_COUNT].monsters[a % 4];
}

static const char *base_name(int b) {
  return (b >= 0 && b < g_base_count) ? g_bases[b].name : "???";
}

void game_log_text(int i, char *buf, size_t size) {
  buf[0] = '\0';
  const LogEntry *e = log_at(i);
  if (!e) return;
  switch ((LogType)e->type) {
    case LOG_SUMMARY: {
      // a=レベル b=戦闘 c=ゴールド d=アイテム
      int n = snprintf(buf, size, "Recap: %d fights +%dG %d items", e->b, e->c, e->d);
      if (e->a && n > 0 && n < (int)size) snprintf(buf + n, size - n, " Lv+%d", e->a);
      break;
    }
    case LOG_WELCOME: snprintf(buf, size, "Welcome to town, hero."); break;
    case LOG_DEPART: snprintf(buf, size, "Entered %s.", g_dungeons[e->a % DUNGEON_COUNT].name); break;
    case LOG_FLOOR: snprintf(buf, size, "Reached floor %d.", e->a); break;
    case LOG_BATTLE:
      snprintf(buf, size, "Slew %s. -%dHP +%dXP +%dG", monster_name(e->a), e->b, e->c, e->d);
      break;
    case LOG_BOSS:
      snprintf(buf, size, "Defeated %s! -%dHP +%dXP +%dG", monster_name(e->a), e->b, e->c, e->d);
      break;
    case LOG_ITEM: snprintf(buf, size, "Found %s.", base_name(e->b)); break;
    case LOG_BAG_FULL: snprintf(buf, size, "Bag full. Left %s.", base_name(e->b)); break;
    case LOG_GOLD: snprintf(buf, size, "Found %d gold.", e->b); break;
    case LOG_TRAP: snprintf(buf, size, "A trap! -%dHP", e->b); break;
    case LOG_FOUNTAIN: snprintf(buf, size, "A fountain. +%dHP", e->b); break;
    case LOG_POTION: snprintf(buf, size, "Drank a potion. +%dHP", e->b); break;
    case LOG_LEVEL: snprintf(buf, size, "Level up! Now Lv %d.", e->a); break;
    case LOG_LOW_HP: snprintf(buf, size, "HP is low. Heading back."); break;
    case LOG_PORTAL: snprintf(buf, size, "Read a portal scroll."); break;
    case LOG_WALK_BACK: snprintf(buf, size, "Turned back for town."); break;
    case LOG_BOTTOM: snprintf(buf, size, "Deepest floor cleared! Heading back."); break;
    case LOG_TOWN: snprintf(buf, size, "Made it back to town."); break;
    case LOG_DEATH:
      snprintf(buf, size, "Died in %s F%d. Gear left behind.", g_dungeons[e->a % DUNGEON_COUNT].name, e->b);
      break;
    case LOG_RECOVER:
      if (e->c) snprintf(buf, size, "Found your lost gear! Took %d, %d left.", e->b, e->c);
      else snprintf(buf, size, "Found your lost gear! Took %d.", e->b);
      break;
    case LOG_DROP_GONE: snprintf(buf, size, "Your lost gear has vanished."); break;
    case LOG_AGENT: snprintf(buf, size, "The agent brought back %d items.", e->b); break;
    case LOG_NO_STEPS: snprintf(buf, size, "No step data. Check Health."); break;
    default: break;
  }
}

// ============================================================
// アイテム
// ============================================================
const BaseDef *game_item_base(const Item *it) {
  if (!it || it->base == 0 || it->base > g_base_count) return NULL;
  return &g_bases[it->base - 1];
}

ItemStats game_item_stats(const Item *it) {
  ItemStats s = { 0, 0, 0 };
  const BaseDef *b = game_item_base(it);
  if (!b) return s;
  int v = it->ilvl;
  int f = 85 + it->seed % 31;   // 性能のばらつき 85〜115%
  switch (b->slot) {
    case SLOT_WEAPON: s.atk = (4 + v * 3 / 2) * f / 100; break;
    case SLOT_OFFHAND: s.def = (2 + v * 3 / 5) * f / 100; s.hp = v; break;
    case SLOT_HEAD: s.def = (1 + v / 2) * f / 100; break;
    case SLOT_BODY: s.def = (2 + v * 4 / 5) * f / 100; s.hp = 2 * v; break;
    case SLOT_HANDS: s.atk = (1 + v / 3) * f / 100; s.def = (v / 3) * f / 100; break;
    case SLOT_FEET: s.def = (1 + v * 2 / 5) * f / 100; break;
    case SLOT_AMULET: s.hp = (5 + v * 3) * f / 100; break;
    default: s.atk = (1 + v / 2) * f / 100; break;   // 指輪
  }
  return s;
}

int game_item_price(const Item *it) {
  if (!game_item_base(it)) return 0;
  int v = it->ilvl;
  return 3 + v * 2 + v * v / 4;
}

static Item make_item(int base, int ilvl) {
  Item it = { 0 };
  it.base = (uint16_t)(base + 1);
  it.seed = (uint16_t)rnd(65536);
  it.ilvl = (uint8_t)(ilvl < 1 ? 1 : ilvl);
  return it;
}

const Item *game_equipped(int slot) {
  return (slot >= 0 && slot < EQUIP_SLOTS && s_equip[slot].base) ? &s_equip[slot] : NULL;
}

int game_bag_count(void) {
  int n = 0;
  while (n < BAG_SIZE && s_bag[n].base) n++;
  return n;
}

const Item *game_bag(int i) {
  return (i >= 0 && i < BAG_SIZE && s_bag[i].base) ? &s_bag[i] : NULL;
}

static bool bag_add(const Item *it) {
  int n = game_bag_count();
  if (n >= BAG_SIZE) return false;
  s_bag[n] = *it;
  s_dirty = true;
  return true;
}

static void bag_remove(int i) {
  int n = game_bag_count();
  if (i < 0 || i >= n) return;
  memmove(&s_bag[i], &s_bag[i + 1], sizeof(Item) * (n - i - 1));
  memset(&s_bag[n - 1], 0, sizeof(Item));
  s_dirty = true;
}

bool game_can_change_gear(void) { return s_run.mode == RUN_NONE; }

bool game_equip_from_bag(int bag_index) {
  if (!game_can_change_gear()) return false;
  const Item *it = game_bag(bag_index);
  const BaseDef *b = game_item_base(it);
  if (!b) return false;
  int slot = b->slot;
  if (slot == SLOT_RING1 && s_equip[SLOT_RING1].base && !s_equip[SLOT_RING2].base) slot = SLOT_RING2;
  Item picked = *it;
  Item old = s_equip[slot];
  s_equip[slot] = picked;
  if (old.base) s_bag[bag_index] = old;   // 外した物は同じ場所へ
  else bag_remove(bag_index);
  game_save();
  return true;
}

bool game_unequip(int slot) {
  if (!game_can_change_gear() || !game_equipped(slot)) return false;
  if (!bag_add(&s_equip[slot])) return false;
  memset(&s_equip[slot], 0, sizeof(Item));
  game_save();
  return true;
}

bool game_sell_bag(int bag_index) {
  if (!game_can_change_gear()) return false;
  const Item *it = game_bag(bag_index);
  if (!it) return false;
  s_hero.gold += game_item_price(it);
  bag_remove(bag_index);
  game_save();
  return true;
}

// ============================================================
// 勇者のステータス
// ============================================================
static ItemStats gear_total(void) {
  ItemStats t = { 0, 0, 0 };
  for (int i = 0; i < EQUIP_SLOTS; i++) {
    if (!s_equip[i].base) continue;
    ItemStats s = game_item_stats(&s_equip[i]);
    t.atk += s.atk;
    t.def += s.def;
    t.hp += s.hp;
  }
  return t;
}

int game_level(void) { return s_hero.level; }
int32_t game_xp(void) { return s_hero.xp; }
int32_t game_xp_next(void) { int l = s_hero.level; return 20 * l + 5 * l * l; }
int32_t game_gold(void) { return s_hero.gold; }
int game_max_hp(void) { return 50 + 12 * s_hero.level + gear_total().hp; }
int game_atk(void) { return 4 + 2 * s_hero.level + gear_total().atk; }
int game_def(void) { return s_hero.level + gear_total().def; }
int game_potions(void) { return s_hero.potions; }
int game_portals(void) { return s_hero.portals; }

int game_hp(void) {
  return s_run.mode == RUN_NONE ? game_max_hp() : s_run.hp;
}

// ============================================================
// 設定
// ============================================================
int game_auto_return_pct(void) { return s_hero.auto_return_pct; }

void game_cycle_auto_return(void) {
  static const uint8_t CHOICES[] = { 0, 20, 30, 40, 50 };
  int n = sizeof(CHOICES);
  int cur = 0;
  for (int i = 0; i < n; i++) {
    if (CHOICES[i] == s_hero.auto_return_pct) cur = i;
  }
  s_hero.auto_return_pct = CHOICES[(cur + 1) % n];
  game_save();
}

bool game_vibrate(void) { return s_hero.vibrate; }

void game_toggle_vibrate(void) {
  s_hero.vibrate = !s_hero.vibrate;
  game_save();
}

// ============================================================
// 冒険
// ============================================================
static const DungeonDef *cur_dungeon(void) { return &g_dungeons[s_run.dungeon % DUNGEON_COUNT]; }

static uint32_t total_steps(const DungeonDef *d) {
  return (uint32_t)d->floors * d->steps_per_floor;
}

// 入口からの距離（帰り道では残り歩数がそのまま入口からの距離になる）
static uint32_t position(void) {
  return s_run.mode == RUN_RETURN ? s_run.return_left : s_run.depth;
}

static int floor_at(uint32_t pos) {
  const DungeonDef *d = cur_dungeon();
  int f = (int)(pos / d->steps_per_floor) + 1;
  return f > d->floors ? d->floors : f;
}

static int monster_level(int floor) {
  const DungeonDef *d = cur_dungeon();
  if (d->floors <= 1) return d->lvl_max;
  return d->lvl_min + (d->lvl_max - d->lvl_min) * (floor - 1) / (d->floors - 1);
}

RunMode game_run_mode(void) { return (RunMode)s_run.mode; }
int game_run_dungeon(void) { return s_run.dungeon; }
int game_floor(void) { return floor_at(position()); }
int game_return_left(void) { return (int)s_run.return_left; }
bool game_steps_available(void) { return steps_available(); }

int game_steps_to_next_floor(void) {
  const DungeonDef *d = cur_dungeon();
  uint32_t total = total_steps(d);
  if (s_run.depth >= total) return 0;
  uint32_t next = (s_run.depth / d->steps_per_floor + 1) * d->steps_per_floor;
  return (int)(next - s_run.depth);
}

bool game_dungeon_cleared(int idx) { return (s_hero.cleared >> idx) & 1; }

bool game_dungeon_unlocked(int idx) {
  return idx == 0 || game_dungeon_cleared(idx - 1);
}

static void add_xp(int xp) {
  if (s_hero.level >= MAX_LEVEL) return;
  s_hero.xp += xp;
  while (s_hero.level < MAX_LEVEL && s_hero.xp >= game_xp_next()) {
    s_hero.xp -= game_xp_next();
    s_hero.level++;
    s_run.hp += 12;   // 最大HPが増えた分だけ回復する
    s_batch.levels++;
    log_push(LOG_LEVEL, s_hero.level, 0, 0, 0);
  }
  if (s_hero.level >= MAX_LEVEL) s_hero.xp = 0;
}

static void heal(int amount) {
  s_run.hp += amount;
  if (s_run.hp > game_max_hp()) s_run.hp = game_max_hp();
}

// HP が 30% を切ったらポーションを飲む
static void maybe_drink(void) {
  int max = game_max_hp();
  if (s_run.hp > 0 && s_run.hp * 100 < max * 30 && s_hero.potions > 0) {
    s_hero.potions--;
    int before = s_run.hp;
    heal(max / 2);
    log_push(LOG_POTION, 0, s_run.hp - before, 0, 0);
  }
}

static void arrive_town(void) {
  s_run.mode = RUN_NONE;
  s_run.hp = game_max_hp();
  if (s_hero.runs < 65535) s_hero.runs++;
  log_push(LOG_TOWN, 0, 0, 0, 0);
}

static bool carrying_anything(void) {
  if (s_run.run_gold > 0 || game_bag_count() > 0) return true;
  for (int i = 0; i < EQUIP_SLOTS; i++) if (s_equip[i].base) return true;
  return false;
}

static void die(void) {
  if (s_hero.deaths < 65535) s_hero.deaths++;
  s_alert = true;
  if (!carrying_anything()) {
    // 何も持っていなければ、前に残した物はそのまま
    log_push(LOG_DEATH, s_run.dungeon, floor_at(position()), 0, 0);
    s_run.mode = RUN_NONE;
    s_run.hp = game_max_hp();
    return;
  }
  // 装備と持ち物、この探索で拾ったゴールドをその場に残す（前に残した物は消える）
  s_drop.active = 1;
  s_drop.dungeon = s_run.dungeon;
  s_drop.depth = position();
  s_drop.time = (uint32_t)time(NULL);
  s_drop.gold = s_run.run_gold;
  memcpy(s_drop_equip, s_equip, sizeof(s_equip));
  memcpy(s_drop_bag, s_bag, sizeof(s_bag));
  memset(s_equip, 0, sizeof(s_equip));
  memset(s_bag, 0, sizeof(s_bag));
  s_hero.gold -= s_run.run_gold;
  if (s_hero.gold < 0) s_hero.gold = 0;
  log_push(LOG_DEATH, s_run.dungeon, floor_at(s_drop.depth), 0, 0);
  s_run.mode = RUN_NONE;
  s_run.hp = game_max_hp();
}

static void start_return(void) {
  if (s_run.depth == 0) {
    // 入口にいるならそのまま町へ
    arrive_town();
    return;
  }
  s_run.mode = RUN_RETURN;
  s_run.return_left = s_run.depth;
  s_run.next_event = s_run.return_left > 90 ? s_run.return_left - 90 : 0;
}

// 自動帰還の判定（奥へ進んでいるときだけ）
static void check_auto_return(void) {
  if (s_run.mode != RUN_EXPLORE || !s_hero.auto_return_pct) return;
  if (s_run.hp * 100 >= game_max_hp() * s_hero.auto_return_pct) return;
  s_alert = true;
  if (s_hero.portals > 0) {
    s_hero.portals--;
    log_push(LOG_PORTAL, 0, 0, 0, 0);
    arrive_town();
  } else {
    log_push(LOG_LOW_HP, 0, 0, 0, 0);
    start_return();
  }
}

// ダメージ計算（攻撃が防御に比べて低いほど減る。最低1）
static int damage(int atk, int def) {
  int sum = atk + def;
  int d = atk * atk / (sum > 0 ? sum : 1);
  d = d * (80 + rnd(41)) / 100;
  return d < 1 ? 1 : d;
}

static void found_item(int ilvl) {
  Item it = make_item(rnd(g_base_count), ilvl);
  if (bag_add(&it)) {
    s_batch.items++;
    log_push(LOG_ITEM, 0, it.base - 1, 0, 0);
  } else {
    log_push(LOG_BAG_FULL, 0, it.base - 1, 0, 0);
  }
}

static void add_gold(int g) {
  s_hero.gold += g;
  s_run.run_gold += g;
  s_batch.gold += g;
}

// 戦闘。勝ったら true（負けたら die() 済み）
static bool battle(int monster, int mlvl, bool boss) {
  // 敵の攻撃は深いダンジョンほど急に強くなる（tools で試算して決めた値）
  int mhp = 10 + 7 * mlvl;
  int matk = 2 + mlvl + mlvl * mlvl / 40;
  int mdef = mlvl;
  if (boss) {
    mhp *= 3;
    matk = matk * 3 / 2;
  }
  int start_hp = s_run.hp;
  int atk = game_atk();
  int def = game_def();
  for (int round = 0; round < 60 && mhp > 0; round++) {
    mhp -= damage(atk, mdef);
    if (mhp <= 0) break;
    s_run.hp -= damage(matk, def);
    if (s_run.hp <= 0) {
      die();
      return false;
    }
    maybe_drink();
  }
  int taken = start_hp - s_run.hp;
  int xp = 4 + 3 * mlvl;
  int gold = 1 + mlvl + rnd(2 * mlvl + 2);
  if (boss) {
    xp *= 3;
    gold *= 4;
  }
  add_gold(gold);
  s_batch.battles++;
  log_push(boss ? LOG_BOSS : LOG_BATTLE, monster, taken, xp, gold);
  add_xp(xp);
  return true;
}

// ダメージを受けた後の処理。死んだら true
static bool hurt(int d) {
  s_run.hp -= d;
  log_push(LOG_TRAP, 0, d, 0, 0);
  if (s_run.hp <= 0) {
    die();
    return true;
  }
  maybe_drink();
  return false;
}

static void fountain(void) {
  int before = s_run.hp;
  heal(game_max_hp() * 40 / 100);
  if (s_run.hp > before) log_push(LOG_FOUNTAIN, 0, s_run.hp - before, 0, 0);
}

static void explore_event(void) {
  int mlvl = monster_level(floor_at(s_run.depth));
  int r = rnd(100);
  if (r < 48) {
    if (battle(s_run.dungeon * 4 + rnd(3), mlvl, false)) {
      if (rnd(100) < 12) found_item(mlvl);
      check_auto_return();
    }
  } else if (r < 63) {
    found_item(mlvl);
  } else if (r < 75) {
    int g = 2 + mlvl * 2 + rnd(mlvl * 3 + 3);
    add_gold(g);
    log_push(LOG_GOLD, 0, g, 0, 0);
  } else if (r < 85) {
    int d = game_max_hp() * (8 + rnd(8)) / 100;
    if (!hurt(d < 1 ? 1 : d)) check_auto_return();
  } else if (r < 93) {
    fountain();
  }
  // 残りは何も起きない
}

// 帰り道の出来事（奥へ進むときより穏やか）
static void return_event(void) {
  int mlvl = monster_level(floor_at(s_run.return_left));
  int r = rnd(100);
  if (r < 35) {
    battle(s_run.dungeon * 4 + rnd(3), mlvl, false);
  } else if (r < 45) {
    int g = 2 + mlvl * 2 + rnd(mlvl * 2 + 2);
    add_gold(g);
    log_push(LOG_GOLD, 0, g, 0, 0);
  } else if (r < 52) {
    int d = game_max_hp() * (6 + rnd(6)) / 100;
    hurt(d < 1 ? 1 : d);
  } else if (r < 58) {
    fountain();
  }
}

// 残した物を、空いている装備枠と持ち物に戻す。戻せた数を返す
static int take_drop(void) {
  int taken = 0;
  for (int i = 0; i < EQUIP_SLOTS; i++) {
    if (!s_drop_equip[i].base) continue;
    if (!s_equip[i].base) s_equip[i] = s_drop_equip[i];
    else if (!bag_add(&s_drop_equip[i])) continue;
    memset(&s_drop_equip[i], 0, sizeof(Item));
    taken++;
  }
  for (int i = 0; i < BAG_SIZE; i++) {
    if (!s_drop_bag[i].base) continue;
    if (!bag_add(&s_drop_bag[i])) continue;
    memset(&s_drop_bag[i], 0, sizeof(Item));
    taken++;
  }
  s_dirty = true;
  return taken;
}

int game_drop_item_count(void) {
  if (!s_drop.active) return 0;
  int n = 0;
  for (int i = 0; i < EQUIP_SLOTS; i++) if (s_drop_equip[i].base) n++;
  for (int i = 0; i < BAG_SIZE; i++) if (s_drop_bag[i].base) n++;
  return n;
}

static void recover_drop(void) {
  s_run.drop_checked = 1;
  int taken = take_drop();
  add_gold(s_drop.gold);
  s_drop.gold = 0;
  int left = game_drop_item_count();
  if (left == 0) s_drop.active = 0;
  log_push(LOG_RECOVER, 0, taken, left, 0);
}

static bool drop_on_path(void) {
  return s_drop.active && !s_run.drop_checked && s_drop.dungeon == s_run.dungeon;
}

static uint32_t event_gap(void) { return 30 + rnd(61); }       // 平均60歩
static uint32_t return_gap(void) { return 45 + rnd(91); }      // 平均90歩

// 歩数 n 歩分、冒険を進める
static void advance(int32_t n) {
  while (n > 0 && s_run.mode != RUN_NONE) {
    const DungeonDef *d = cur_dungeon();
    uint32_t total = total_steps(d);
    if (s_run.mode == RUN_EXPLORE) {
      // 次に何かが起きる位置（出来事・階の境目・残した物の場所・最深部）まで進む
      uint32_t target = s_run.next_event < total ? s_run.next_event : total;
      uint32_t next_floor = (s_run.depth / d->steps_per_floor + 1) * d->steps_per_floor;
      if (next_floor < target) target = next_floor;
      if (drop_on_path() && s_drop.depth > s_run.depth && s_drop.depth < target) target = s_drop.depth;
      uint32_t step = target - s_run.depth;
      if ((int32_t)step > n) step = (uint32_t)n;
      s_run.depth += step;
      n -= (int32_t)step;
      if (s_run.depth < target) break;

      if (s_run.depth % d->steps_per_floor == 0 && s_run.depth < total) {
        log_push(LOG_FLOOR, floor_at(s_run.depth), 0, 0, 0);
      }
      if (drop_on_path() && s_drop.depth <= s_run.depth) recover_drop();
      if (s_run.depth >= total) {
        // 最深部：ボスと戦い、勝ったら帰り始める
        if (!s_run.boss_done) {
          if (!battle(s_run.dungeon * 4 + 3, d->lvl_max, true)) break;
          s_run.boss_done = 1;
          s_hero.cleared |= 1 << s_run.dungeon;
          found_item(d->lvl_max);
        }
        log_push(LOG_BOTTOM, 0, 0, 0, 0);
        start_return();
        continue;
      }
      if (s_run.depth >= s_run.next_event) {
        explore_event();
        // 自動帰還で帰り道に切り替わった場合は、帰り道の予定をそのまま使う
        if (s_run.mode == RUN_EXPLORE) s_run.next_event = s_run.depth + event_gap();
      }
    } else {
      uint32_t stop = s_run.next_event;
      if (drop_on_path() && s_drop.depth < s_run.return_left && s_drop.depth > stop) stop = s_drop.depth;
      uint32_t step = s_run.return_left - stop;
      if ((int32_t)step > n) step = (uint32_t)n;
      s_run.return_left -= step;
      n -= (int32_t)step;
      if (s_run.return_left > stop) break;

      // 帰り道で残した物の場所をちょうど通ったとき（奥へ進む途中で通った場合は回収済み）
      if (drop_on_path() && s_drop.depth == s_run.return_left) recover_drop();
      if (s_run.return_left == 0) {
        arrive_town();
        break;
      }
      if (s_run.return_left <= s_run.next_event) {
        return_event();
        uint32_t gap = return_gap();
        s_run.next_event = s_run.return_left > gap ? s_run.return_left - gap : 0;
      }
    }
  }
  s_dirty = true;
}

bool game_depart(int idx) {
  if (idx < 0 || idx >= DUNGEON_COUNT || s_run.mode != RUN_NONE) return false;
  if (!game_dungeon_unlocked(idx)) return false;
  s_run.mode = RUN_EXPLORE;
  s_run.dungeon = (uint8_t)idx;
  s_run.boss_done = 0;
  s_run.drop_checked = 0;
  s_run.hp = game_max_hp();
  s_run.depth = 0;
  s_run.return_left = 0;
  s_run.rng = ((uint32_t)time(NULL) * 2654435761u) ^ (uint32_t)rand();
  s_run.next_event = event_gap();
  s_run.run_gold = 0;
  steps_snapshot(&s_run.snap);
  s_warned_no_steps = false;
  log_push(LOG_DEPART, idx, 0, 0, 0);
  game_save();
  return true;
}

bool game_use_portal(void) {
  game_update();   // それまでに歩いた分を先に反映する
  if (s_run.mode == RUN_NONE || s_hero.portals == 0) return false;
  s_hero.portals--;
  log_push(LOG_PORTAL, 0, 0, 0, 0);
  arrive_town();
  game_save();
  return true;
}

void game_walk_back(void) {
  game_update();
  if (s_run.mode != RUN_EXPLORE) return;
  log_push(LOG_WALK_BACK, 0, 0, 0, 0);
  start_return();
  game_save();
}

// ============================================================
// お店
// ============================================================
Item game_shop_gear(int i) {
  Item it = { 0 };
  it.base = (uint16_t)(i % g_base_count + 1);
  it.ilvl = (uint8_t)s_hero.level;
  it.seed = (uint16_t)((s_hero.level * 131 + i * 17) & 0xFFFF);
  return it;
}

int game_shop_gear_cost(int i) {
  Item it = game_shop_gear(i);
  return game_item_price(&it) * 4;
}

BuyResult game_buy_gear(int i) {
  int cost = game_shop_gear_cost(i);
  if (s_hero.gold < cost) return BUY_NO_GOLD;
  if (game_bag_count() >= BAG_SIZE) return BUY_FULL;
  Item it = game_shop_gear(i);
  bag_add(&it);
  s_hero.gold -= cost;
  game_save();
  return BUY_OK;
}

BuyResult game_buy_potion(void) {
  if (s_hero.potions >= MAX_POTIONS) return BUY_FULL;
  if (s_hero.gold < POTION_PRICE) return BUY_NO_GOLD;
  s_hero.gold -= POTION_PRICE;
  s_hero.potions++;
  game_save();
  return BUY_OK;
}

BuyResult game_buy_portal(void) {
  if (s_hero.portals >= MAX_PORTALS) return BUY_FULL;
  if (s_hero.gold < PORTAL_PRICE) return BUY_NO_GOLD;
  s_hero.gold -= PORTAL_PRICE;
  s_hero.portals++;
  game_save();
  return BUY_OK;
}

// ============================================================
// 死亡時に残した物・回収代行
// ============================================================
bool game_drop_exists(void) { return s_drop.active; }
int game_drop_dungeon(void) { return s_drop.dungeon; }

int game_drop_floor(void) {
  const DungeonDef *d = &g_dungeons[s_drop.dungeon % DUNGEON_COUNT];
  int f = (int)(s_drop.depth / d->steps_per_floor) + 1;
  return f > d->floors ? d->floors : f;
}

int32_t game_drop_seconds_left(void) {
  if (!s_drop.active) return 0;
  int32_t left = (int32_t)(s_drop.time + DROP_EXPIRE_SEC) - (int32_t)time(NULL);
  return left < 0 ? 0 : left;
}

int32_t game_drop_fee(void) {
  int32_t value = 0;
  for (int i = 0; i < EQUIP_SLOTS; i++) value += game_item_price(&s_drop_equip[i]);
  for (int i = 0; i < BAG_SIZE; i++) value += game_item_price(&s_drop_bag[i]);
  return 100 * (s_drop.dungeon + 1) + value / 4;
}

AgentResult game_agent_check(void) {
  if (!s_drop.active || s_run.mode != RUN_NONE) return AGENT_NONE;
  if (s_hero.gold < game_drop_fee()) return AGENT_NO_GOLD;
  // 全部を持ち帰れるだけの空きが必要
  int need = 0;
  for (int i = 0; i < EQUIP_SLOTS; i++) if (s_drop_equip[i].base && s_equip[i].base) need++;
  for (int i = 0; i < BAG_SIZE; i++) if (s_drop_bag[i].base) need++;
  if (game_bag_count() + need > BAG_SIZE) return AGENT_NO_ROOM;
  return AGENT_OK;
}

AgentResult game_hire_agent(void) {
  AgentResult check = game_agent_check();
  if (check != AGENT_OK) return check;
  s_hero.gold -= game_drop_fee();
  int taken = take_drop();
  s_hero.gold += s_drop.gold;
  s_drop.active = 0;
  s_drop.gold = 0;
  log_push(LOG_AGENT, 0, taken, 0, 0);
  game_save();
  return AGENT_OK;
}

static void check_drop_expire(void) {
  if (s_drop.active && game_drop_seconds_left() == 0) {
    s_drop.active = 0;
    s_drop.gold = 0;
    memset(s_drop_equip, 0, sizeof(s_drop_equip));
    memset(s_drop_bag, 0, sizeof(s_drop_bag));
    log_push(LOG_DROP_GONE, 0, 0, 0, 0);
  }
}

// ============================================================
// 更新・保存・読み込み
// ============================================================
bool game_update(void) {
  s_alert = false;
  check_drop_expire();
  if (s_run.mode != RUN_NONE) {
    if (!steps_available()) {
      if (!s_warned_no_steps) {
        s_warned_no_steps = true;
        log_push(LOG_NO_STEPS, 0, 0, 0, 0);
      }
    } else {
      int32_t n = steps_since(&s_run.snap);
      if (n > 0) {
        memset(&s_batch, 0, sizeof(s_batch));
        advance(n);
        // 一度にたくさん起きたとき（アプリを閉じていた間など）は、まとめを一番新しいログに出す
        if (s_batch.events > LOG_SUMMARY_MIN) {
          log_push(LOG_SUMMARY, s_batch.levels, s_batch.battles, s_batch.gold, s_batch.items);
        }
      }
    }
  }
  bool changed = s_dirty;
  if (s_dirty) game_save();
  if (s_alert && s_hero.vibrate) vibes_double_pulse();
  return changed;
}

void game_save(void) {
  persist_write_data(KEY_HERO, &s_hero, sizeof(s_hero));
  persist_write_data(KEY_RUN, &s_run, sizeof(s_run));
  persist_write_data(KEY_EQUIP, s_equip, sizeof(s_equip));
  persist_write_data(KEY_BAG, s_bag, sizeof(s_bag));
  persist_write_data(KEY_LOG, &s_log, sizeof(s_log));
  persist_write_data(KEY_DROP, &s_drop, sizeof(s_drop));
  persist_write_data(KEY_DROP_EQUIP, s_drop_equip, sizeof(s_drop_equip));
  persist_write_data(KEY_DROP_BAG, s_drop_bag, sizeof(s_drop_bag));
  s_dirty = false;
}

static void new_game(void) {
  memset(&s_hero, 0, sizeof(s_hero));
  memset(&s_run, 0, sizeof(s_run));
  memset(s_equip, 0, sizeof(s_equip));
  memset(s_bag, 0, sizeof(s_bag));
  memset(&s_log, 0, sizeof(s_log));
  s_log.layout = LOG_LAYOUT_LINEAR;
  memset(&s_drop, 0, sizeof(s_drop));
  memset(s_drop_equip, 0, sizeof(s_drop_equip));
  memset(s_drop_bag, 0, sizeof(s_drop_bag));
  s_hero.version = SAVE_VERSION;
  s_hero.level = 1;
  s_hero.gold = 100;
  s_hero.potions = 3;
  s_hero.portals = 1;
  s_hero.auto_return_pct = 30;
  s_run.rng = (uint32_t)time(NULL);
  // 最初の装備
  s_equip[SLOT_WEAPON] = make_item(0, 1);
  s_equip[SLOT_BODY] = make_item(3, 1);
  log_push(LOG_WELCOME, 0, 0, 0, 0);
  game_save();
}

void game_init(void) {
  srand((unsigned int)time(NULL));
  // 旧バージョンのセーブは使わない
  if (persist_exists(KEY_OLD_V1)) persist_delete(KEY_OLD_V1);
  if (persist_exists(KEY_OLD_V2)) persist_delete(KEY_OLD_V2);

  if (!persist_exists(KEY_HERO)) {
    new_game();
    return;
  }
  persist_read_data(KEY_HERO, &s_hero, sizeof(s_hero));
  if (s_hero.version != SAVE_VERSION) {
    new_game();
    return;
  }
  persist_read_data(KEY_RUN, &s_run, sizeof(s_run));
  persist_read_data(KEY_EQUIP, s_equip, sizeof(s_equip));
  persist_read_data(KEY_BAG, s_bag, sizeof(s_bag));
  persist_read_data(KEY_LOG, &s_log, sizeof(s_log));
  persist_read_data(KEY_DROP, &s_drop, sizeof(s_drop));
  persist_read_data(KEY_DROP_EQUIP, s_drop_equip, sizeof(s_drop_equip));
  persist_read_data(KEY_DROP_BAG, s_drop_bag, sizeof(s_drop_bag));
  // 壊れたデータへの保険
  if (s_hero.level < 1 || s_hero.level > MAX_LEVEL) s_hero.level = 1;
  if (s_run.mode > RUN_RETURN) s_run.mode = RUN_NONE;
  if (s_run.dungeon >= DUNGEON_COUNT) s_run.dungeon = 0;
  if (s_log.count > LOG_SIZE) memset(&s_log, 0, sizeof(s_log));
  if (s_log.layout != LOG_LAYOUT_LINEAR) {
    // 旧形式（輪になったバッファ。layout に次の書き込み位置が入っている）を古い順に並べ直す
    LogBuf old = s_log;
    int start = old.count < LOG_SIZE ? 0 : old.layout % LOG_SIZE;
    for (int i = 0; i < old.count; i++) s_log.entries[i] = old.entries[(start + i) % LOG_SIZE];
    s_log.layout = LOG_LAYOUT_LINEAR;
  }
}
