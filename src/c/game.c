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
#define KEY_STASH_A 108   // 保管庫の前半 24個（192バイト）
#define KEY_STASH_B 109   // 保管庫の後半 24個
#define KEY_CODEX 110     // 図鑑の発見ビット（128バイト）
#define KEY_CODEX_SEEN 111   // 噂などで見ただけのビット（128バイト）
#define KEY_RUMOUR 112       // 狙っている品（図鑑の番号）
#define KEY_BOSS_MISS 113    // ボス専用ユニークの空振り回数
#define SAVE_VERSION 2   // 2: 図鑑1000種の番号に変更（それ以前のセーブはリセット）

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

const char *const g_slot_names[EQUIP_SLOTS] = {
  "Weapon", "Shield", "Head", "Body", "Hands", "Feet", "Amulet", "Ring", "Ring",
};

// 接尾辞・接頭辞（画面が狭いので短い単語だけ）
const AffixDef g_prefixes[] = {
  { "Sharp",   AFFIX_ATK, 100 },
  { "Cruel",   AFFIX_ATK, 150 },
  { "Savage",  AFFIX_ATK, 200 },
  { "Sturdy",  AFFIX_DEF, 100 },
  { "Plated",  AFFIX_DEF, 150 },
  { "Adamant", AFFIX_DEF, 200 },
  { "Hale",    AFFIX_HP,  100 },
  { "Vital",   AFFIX_HP,  150 },
  { "Titan",   AFFIX_HP,  200 },
};
const int g_prefix_count = sizeof(g_prefixes) / sizeof(g_prefixes[0]);

const AffixDef g_suffixes[] = {
  { "Power",  AFFIX_ATK, 120 },
  { "Fury",   AFFIX_ATK, 180 },
  { "Wrath",  AFFIX_ATK, 240 },
  { "Guard",  AFFIX_DEF, 120 },
  { "Stone",  AFFIX_DEF, 180 },
  { "Aegis",  AFFIX_DEF, 240 },
  { "Life",   AFFIX_HP,  120 },
  { "Giants", AFFIX_HP,  180 },
  { "Dragon", AFFIX_HP,  240 },
};
const int g_suffix_count = sizeof(g_suffixes) / sizeof(g_suffixes[0]);

const char *const g_rarity_names[RARITY_COUNT] = {
  "Normal", "Magic", "Rare", "Set", "Unique",
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
  uint8_t scrolls_id;       // 鑑定の巻物
  uint16_t runs;
  uint16_t deaths;
  uint8_t remind_hour;      // 出発の知らせ（0 = しない）
  uint8_t sleep_tier;       // 今日の睡眠のボーナス（SleepTier）
  uint16_t sleep_min;       // 昨夜眠った分数
  int32_t saved_steps;      // 町にいる間に貯めた歩数
  uint32_t sleep_day;       // sleep_tier を決めた日の 0時
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
static Item s_stash[STASH_SIZE];
static uint8_t s_codex[CODEX_BYTES];        // 自分で手に入れた
static uint8_t s_codex_seen[CODEX_BYTES];   // 噂で知った・取り逃した
static uint16_t s_rumours[RUMOUR_SLOTS];    // 狙っている品（図鑑の番号 + 1。0 は空き）
static uint8_t s_boss_miss[DUNGEON_COUNT];  // ボスを倒しても専用ユニークが出なかった回数
static bool s_warned_no_steps;
static bool s_low_hp_alerted;   // 帰り道で「HPが少ない」と知らせたか（保存しない）

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
    case LOG_SLEEP:
    case LOG_SAVED_STEPS:
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
    case LOG_IDENTIFY:
    case LOG_SLEEP:
      return LOG_STYLE_GREAT;
    case LOG_ITEM:
      return e->a >= RARITY_RARE ? LOG_STYLE_GREAT : LOG_STYLE_DUNGEON;
    case LOG_UPGRADE:
      return e->c ? LOG_STYLE_GREAT : LOG_STYLE_DANGER;
    case LOG_WELCOME:
    case LOG_TOWN:
    case LOG_SAVED_STEPS:
      return LOG_STYLE_TOWN;
    default:
      return LOG_STYLE_DUNGEON;
  }
}

static const char *monster_name(int a) {
  return g_dungeons[(a / 4) % DUNGEON_COUNT].monsters[a % 4];
}

// ログ用: base 番号（0始まり）からの名前。1回の snprintf の中で1度だけ使うこと
static const char *base_name(int b) {
  static char buf[32];
  Item probe = { 0 };
  probe.base = (uint16_t)(b + 1);
  probe.ilvl = 1;
  game_item_short_name(&probe, buf, sizeof(buf));
  return buf;
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
    case LOG_ITEM:
      if (e->a >= RARITY_RARE) {
        snprintf(buf, size, "Found a %s %s! Unidentified.", g_rarity_names[e->a % RARITY_COUNT],
                 base_name(e->b));
      } else if (e->a == RARITY_MAGIC) {
        snprintf(buf, size, "Found a magic %s.", base_name(e->b));
      } else {
        snprintf(buf, size, "Found %s.", base_name(e->b));
      }
      break;
    case LOG_IDENTIFY:
      snprintf(buf, size, "Identified: %s %s.", g_rarity_names[e->a % RARITY_COUNT], base_name(e->b));
      break;
    case LOG_UPGRADE:
      if (e->c) snprintf(buf, size, "Smith: %s is now +%d!", base_name(e->b), e->a);
      else snprintf(buf, size, "Smith: +%d on %s failed.", e->a, base_name(e->b));
      break;
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
    case LOG_SAVED_STEPS: snprintf(buf, size, "Saved steps: %d used.", e->b); break;
    case LOG_SLEEP:
      snprintf(buf, size, e->a >= SLEEP_REFRESHED ? "Slept %dh%02dm. Refreshed! XP/Gold +20%%"
                                                  : "Slept %dh%02dm. Rested. XP/Gold +10%%",
               e->b / 60, e->b % 60);
      break;
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
//   base 1〜900: 形 × 素材（(base-1) / 6 が形、(base-1) % 6 が素材の段階）
//   base 901〜: 固有・セット装備（g_specials の添字 + 901）
// ============================================================
int game_item_special(const Item *it) {
  if (!it || it->base < SPECIAL_BASE) return -1;
  int idx = it->base - SPECIAL_BASE;
  return idx < g_special_count ? idx : -1;
}

const ShapeDef *game_item_shape(const Item *it) {
  if (!it || it->base == 0) return NULL;
  int special = game_item_special(it);
  if (special >= 0) return &g_shapes[g_specials[special].shape];
  if (it->base > BASE_COUNT) return NULL;
  return &g_shapes[(it->base - 1) / TIER_COUNT];
}

int game_item_tier(const Item *it) {
  if (!it || it->base == 0) return 0;
  if (it->base <= BASE_COUNT) return (it->base - 1) % TIER_COUNT;
  return item_tier_for_level(it->ilvl);
}

bool game_item_identified(const Item *it) {
  if (!it) return false;
  // 白・青は最初から鑑定済み扱い
  return it->rarity < RARITY_RARE || (it->flags & ITEM_FLAG_IDENTIFIED);
}

// seed から接辞などを決めるための値。k を変えると別の値になる
static uint32_t item_hash(const Item *it, int k) {
  uint32_t h = it->seed * 2654435761u + it->base * 40503u + it->ilvl * 97u + k * 2246822519u;
  h ^= h >> 13;
  h *= 3266489917u;
  h ^= h >> 16;
  return h;
}

int game_item_affix_count(const Item *it) {
  if (!it || !game_item_identified(it)) return 0;
  switch (it->rarity) {
    case RARITY_MAGIC: return 1 + (int)(item_hash(it, 1) % 2);
    case RARITY_RARE: return 3 + (int)(item_hash(it, 1) % 2);
    default: return 0;
  }
}

const AffixDef *game_item_affix(const Item *it, int i, int *value) {
  if (i < 0 || i >= game_item_affix_count(it)) return NULL;
  // 偶数番目は接頭辞、奇数番目は接尾辞
  const AffixDef *table = (i % 2 == 0) ? g_prefixes : g_suffixes;
  int count = (i % 2 == 0) ? g_prefix_count : g_suffix_count;
  const AffixDef *a = &table[item_hash(it, 10 + i) % count];
  if (value) {
    int v = it->ilvl;
    int base;
    switch (a->stat) {
      case AFFIX_ATK: base = 1 + v * 2 / 5; break;
      case AFFIX_DEF: base = 1 + v / 3; break;
      default: base = 3 + v * 3 / 2; break;
    }
    int out = base * a->power / 100;
    *value = out < 1 ? 1 : out;
  }
  return a;
}

const SetDef *game_item_set(const Item *it) {
  int special = game_item_special(it);
  if (special < 0) return NULL;
  int id = g_specials[special].set_id;
  return (id >= 1 && id <= g_set_count) ? &g_sets[id - 1] : NULL;
}

// 同じセットの、違う部位を何個つけているか（同じ指輪を2つ着けても1部位）
int game_set_pieces_equipped(const Item *it) {
  const SetDef *set = game_item_set(it);
  if (!set) return 0;
  int n = 0;
  for (int i = 0; i < EQUIP_SLOTS; i++) {
    if (game_item_set(&s_equip[i]) != set || !game_item_identified(&s_equip[i])) continue;
    bool duplicate = false;
    for (int j = 0; j < i; j++) {
      if (s_equip[j].base == s_equip[i].base) duplicate = true;
    }
    if (!duplicate) n++;
  }
  return n;
}

// 効果ごとの上限（強くなりすぎないように）
static const uint8_t FX_CAP[FX_COUNT] = {
  [FX_STRIDE] = 100, [FX_GOLD] = 200, [FX_MAGIC] = 200, [FX_XP] = 100, [FX_LEECH] = 15,
  [FX_GUARD] = 60, [FX_TRAP] = 90, [FX_POTION] = 100, [FX_ATK] = 100, [FX_DEF] = 100,
  [FX_HP] = 100, [FX_ALL] = 50,
};

int game_effect_total(Effect fx) {
  if (fx <= FX_NONE || fx >= FX_COUNT) return 0;
  int total = 0;
  uint32_t sets_done = 0;   // 同じセットのボーナスは1回だけ数える
  for (int i = 0; i < EQUIP_SLOTS; i++) {
    const Item *it = &s_equip[i];
    int special = game_item_special(it);
    if (special < 0 || !game_item_identified(it)) continue;
    const SpecialDef *d = &g_specials[special];
    if (d->fx1 == fx) total += d->val1;
    if (d->fx2 == fx) total += d->val2;
    const SetDef *set = game_item_set(it);
    if (set && !(sets_done & (1u << d->set_id))) {
      sets_done |= 1u << d->set_id;
      int pieces = game_set_pieces_equipped(it);
      if (pieces >= 2 && set->fx2 == fx) total += set->val2;
      if (pieces >= 3 && set->fx3 == fx) total += set->val3;
    }
  }
  return total > FX_CAP[fx] ? FX_CAP[fx] : total;
}

void game_effect_text(Effect fx, int value, char *buf, size_t size) {
  if (fx <= FX_NONE || fx >= FX_COUNT) {
    buf[0] = '\0';
    return;
  }
  switch (fx) {
    case FX_LEECH:
    case FX_GUARD: snprintf(buf, size, "%s %d%%", g_effect_names[fx], value); break;
    case FX_TRAP: snprintf(buf, size, "Trap -%d%%", value); break;
    default: snprintf(buf, size, "%s +%d%%", g_effect_names[fx], value); break;
  }
}

// 接辞・強化を含まない、形と素材による基本の性能
static ItemStats base_stats(const Item *it) {
  ItemStats s = { 0, 0, 0 };
  const ShapeDef *sh = game_item_shape(it);
  if (!sh) return s;
  int v = it->ilvl;
  int f = 85 + it->seed % 31;   // 性能のばらつき 85〜115%
  switch (sh->slot) {
    case SLOT_WEAPON: s.atk = (4 + v * 3 / 2) * f / 100; break;
    case SLOT_OFFHAND: s.def = (2 + v * 3 / 5) * f / 100; s.hp = v; break;
    case SLOT_HEAD: s.def = (1 + v / 2) * f / 100; break;
    case SLOT_BODY: s.def = (2 + v * 4 / 5) * f / 100; s.hp = 2 * v; break;
    case SLOT_HANDS: s.atk = (1 + v / 3) * f / 100; s.def = (v / 3) * f / 100; break;
    case SLOT_FEET: s.def = (1 + v * 2 / 5) * f / 100; break;
    case SLOT_AMULET: s.hp = (5 + v * 3) * f / 100; break;
    default: s.atk = (1 + v / 2) * f / 100; break;   // 指輪
  }
  // 形ごとの傾向（例: Claymore は攻撃が高い、Robe は HP が高い）
  s.atk = s.atk * sh->atk_pct / 100;
  s.def = s.def * sh->def_pct / 100;
  s.hp = s.hp * sh->hp_pct / 100;
  // 低いレベルでも、その装備枠の主な性能は最低 1 にする（性能 0 の装備を出さない）
  switch (sh->slot) {
    case SLOT_WEAPON: case SLOT_HANDS: case SLOT_RING1: case SLOT_RING2:
      if (s.atk < 1) s.atk = 1;
      break;
    case SLOT_AMULET:
      if (s.hp < 1) s.hp = 1;
      break;
    default:
      if (s.def < 1) s.def = 1;
      break;
  }
  return s;
}

ItemStats game_item_stats(const Item *it) {
  ItemStats s = base_stats(it);
  if (!game_item_shape(it)) return s;
  if (!game_item_identified(it)) return s;   // 未鑑定は基本性能しか分からない

  int special = game_item_special(it);
  if (special >= 0) {
    int pct = g_specials[special].bonus_pct;
    s.atk = s.atk * pct / 100;
    s.def = s.def * pct / 100;
    s.hp = s.hp * pct / 100;
  }
  for (int i = 0; i < game_item_affix_count(it); i++) {
    int v = 0;
    const AffixDef *a = game_item_affix(it, i, &v);
    if (!a) continue;
    if (a->stat == AFFIX_ATK) s.atk += v;
    else if (a->stat == AFFIX_DEF) s.def += v;
    else s.hp += v;
  }
  // 強化（+1 ごとに基本性能の 8%）
  if (it->plus) {
    ItemStats b = base_stats(it);
    s.atk += b.atk * 8 * it->plus / 100;
    s.def += b.def * 8 * it->plus / 100;
    s.hp += b.hp * 8 * it->plus / 100;
  }
  return s;
}

void game_item_short_name(const Item *it, char *buf, size_t size) {
  const ShapeDef *sh = game_item_shape(it);
  if (!sh) {
    snprintf(buf, size, "---");
    return;
  }
  char material[ITEM_NAME_LEN], shape[ITEM_NAME_LEN];
  int special = game_item_special(it);
  if (special >= 0 && game_item_identified(it)) {
    item_special_name(special, shape);
    snprintf(buf, size, "%s", shape);
    return;
  }
  item_material_name(sh->family, game_item_tier(it), material);
  item_shape_name((int)(sh - g_shapes), shape);
  snprintf(buf, size, "%s %s", material, shape);
}

void game_item_name(const Item *it, char *buf, size_t size) {
  char short_name[32];
  game_item_short_name(it, short_name, sizeof(short_name));
  int n = 0;
  if (it && it->plus) n += snprintf(buf, size, "+%d ", it->plus);
  const AffixDef *prefix = game_item_affix(it, 0, NULL);
  if (prefix && n < (int)size) n += snprintf(buf + n, size - n, "%s ", prefix->name);
  if (n < (int)size) n += snprintf(buf + n, size - n, "%s", short_name);
  const AffixDef *suffix = game_item_affix(it, 1, NULL);
  if (suffix && n < (int)size) snprintf(buf + n, size - n, " of %s", suffix->name);
}

int game_item_price(const Item *it) {
  if (!game_item_shape(it)) return 0;
  static const uint8_t RARITY_MUL[RARITY_COUNT] = { 10, 18, 30, 50, 80 };
  int v = it->ilvl;
  int price = (3 + v * 2 + v * v / 4) * RARITY_MUL[it->rarity % RARITY_COUNT] / 10;
  if (!game_item_identified(it)) price = price / 2;   // 未鑑定は買い叩かれる
  return price + price * it->plus / 10;
}

// ============================================================
// 睡眠のボーナス
//   昨夜 6時間以上眠れば Rested（経験値・ゴールド +10%）、
//   7時間以上なら Refreshed（経験値・ゴールド +20%、マジック発見 +10）。その日の 0時〜24時だけ効く
// ============================================================
SleepTier game_sleep_tier(void) {
  if (s_hero.sleep_day != (uint32_t)steps_day_start(time(NULL))) return SLEEP_NONE;
  return s_hero.sleep_tier <= SLEEP_REFRESHED ? (SleepTier)s_hero.sleep_tier : SLEEP_NONE;
}

int game_sleep_minutes(void) { return game_sleep_tier() ? s_hero.sleep_min : 0; }

static int sleep_bonus(Effect fx) {
  SleepTier tier = game_sleep_tier();
  switch (fx) {
    case FX_XP:
    case FX_GOLD: return tier == SLEEP_REFRESHED ? 20 : (tier == SLEEP_RESTED ? 10 : 0);
    case FX_MAGIC: return tier == SLEEP_REFRESHED ? 10 : 0;
    default: return 0;
  }
}

// 睡眠の記録を見てボーナスを決める。朝まだ眠っている間に見ても、後で眠った時間が伸びれば上げ直す
// ボーナスが上がったら true（ログは歩数を反映した後に出すので、ここでは出さない）
static bool check_sleep(void) {
  static time_t s_checked_at;   // 時計の記録を見に行くのは数分に1回まで
  time_t now = time(NULL);
  uint32_t today = (uint32_t)steps_day_start(now);
  if (s_hero.sleep_day != today) {
    s_hero.sleep_day = today;
    s_hero.sleep_tier = SLEEP_NONE;
    s_hero.sleep_min = 0;
    s_checked_at = 0;
    s_dirty = true;
  }
  if (s_hero.sleep_tier >= SLEEP_REFRESHED) return false;
  if (s_checked_at && now - s_checked_at < 5 * SECONDS_PER_MINUTE) return false;
  s_checked_at = now;
  int32_t sec = steps_last_night_sleep();
  SleepTier tier = sec >= SLEEP_REFRESHED_SEC ? SLEEP_REFRESHED : (sec >= SLEEP_RESTED_SEC ? SLEEP_RESTED : SLEEP_NONE);
  if (tier > s_hero.sleep_tier) {
    s_hero.sleep_tier = (uint8_t)tier;
    s_hero.sleep_min = (uint16_t)(sec / 60);
    s_dirty = true;
    return true;
  }
  return false;
}

// レア度を決める（深いダンジョンほど、マジック発見が高いほど良い物が出る）
static uint8_t roll_rarity(int dungeon) {
  uint32_t r = (uint32_t)rnd(1000) * 100 / (uint32_t)(100 + game_effect_total(FX_MAGIC) + sleep_bonus(FX_MAGIC));
  if (r < 4 + (uint32_t)dungeon / 2) return RARITY_UNIQUE;
  if (r < 16 + (uint32_t)dungeon) return RARITY_SET;
  if (r < 90 + (uint32_t)dungeon * 5) return RARITY_RARE;
  if (r < 360 + (uint32_t)dungeon * 8) return RARITY_MAGIC;
  return RARITY_NORMAL;
}

static Item make_item(int shape, int ilvl) {
  Item it = { 0 };
  if (ilvl < 1) ilvl = 1;
  it.base = (uint16_t)(shape * TIER_COUNT + item_tier_for_level(ilvl) + 1);
  it.seed = (uint16_t)rnd(65536);
  it.ilvl = (uint8_t)ilvl;
  it.rarity = RARITY_NORMAL;
  it.flags = ITEM_FLAG_IDENTIFIED;
  return it;
}

// 装備枠を均等に選び、その中から形を選ぶ（格の高い形ほど出にくい）
static const uint8_t DROP_SLOTS[] = {
  SLOT_WEAPON, SLOT_OFFHAND, SLOT_HEAD, SLOT_BODY, SLOT_HANDS, SLOT_FEET, SLOT_AMULET, SLOT_RING1,
};
static const uint8_t RANK_WEIGHT[4] = { 8, 5, 3, 2 };

static int pick_shape(int slot) {
  int total = 0;
  for (int s = 0; s < SHAPE_COUNT; s++) {
    if (g_shapes[s].slot == slot) total += RANK_WEIGHT[g_shapes[s].rank & 3];
  }
  int r = rnd(total);
  for (int s = 0; s < SHAPE_COUNT; s++) {
    if (g_shapes[s].slot != slot) continue;
    r -= RANK_WEIGHT[g_shapes[s].rank & 3];
    if (r < 0) return s;
  }
  return SH_SHORT_SWORD;
}

// 固有・セット装備を選ぶ。そのダンジョンの物が出やすい。ボス専用の物は出ない
static int pick_special(int dungeon, bool want_set) {
  int total = 0;
  for (int i = 0; i < g_special_count; i++) {
    const SpecialDef *d = &g_specials[i];
    if ((d->set_id != 0) != want_set || d->boss) continue;
    total += d->dungeon == dungeon ? 8 : (d->dungeon == ANY_DUNGEON ? 4 : 1);
  }
  if (total == 0) return -1;
  int r = rnd(total);
  for (int i = 0; i < g_special_count; i++) {
    const SpecialDef *d = &g_specials[i];
    if ((d->set_id != 0) != want_set || d->boss) continue;
    r -= d->dungeon == dungeon ? 8 : (d->dungeon == ANY_DUNGEON ? 4 : 1);
    if (r < 0) return i;
  }
  return -1;
}

static Item make_special_item(int special, int ilvl) {
  Item it = make_item(g_specials[special].shape, ilvl);
  it.base = (uint16_t)(SPECIAL_BASE + special);
  it.rarity = g_specials[special].set_id ? RARITY_SET : RARITY_UNIQUE;
  it.flags = 0;   // 未鑑定で落ちる
  return it;
}

// 噂の効き目: 狙っている品がこの場所で出るなら、この確率でその品にする
#define RUMOUR_BASE_PCT 30      // 基本アイテム
#define RUMOUR_SPECIAL_PCT 8    // 固有・セット装備
#define RUMOUR_BOSS_PCT 50      // ボス専用ユニーク（ふだんは BOSS_UNIQUE_CHANCE）
#define BOSS_PITY 5             // これだけ空振りしたら次は必ず出す

// ここで出せる噂の品を1つ選ぶ（なければ -1）。基本アイテムは素材の段階が合うことが条件
static int rumour_here(int ilvl, int dungeon) {
  int tier = item_tier_for_level(ilvl);
  int hits[RUMOUR_SLOTS];
  int n = 0;
  for (int i = 0; i < RUMOUR_SLOTS; i++) {
    int entry = game_rumour_at(i);
    if (entry < 0) continue;
    if (entry < BASE_COUNT) {
      if (entry % TIER_COUNT == tier) hits[n++] = entry;
    } else {
      const SpecialDef *d = &g_specials[entry - BASE_COUNT];
      if (d->boss) continue;   // ボス専用は最深部でしか出ない
      if (d->dungeon == dungeon || d->dungeon >= DUNGEON_COUNT) hits[n++] = entry;
    }
  }
  return n ? hits[rnd(n)] : -1;
}

// ダンジョンで拾ったアイテムを作る（レア度つき）
static Item make_drop_item(int ilvl, int dungeon) {
  // 狙っている品が出る場所なら、ときどきその品が出る
  int wanted = rumour_here(ilvl, dungeon);
  if (wanted >= 0) {
    bool special = wanted >= BASE_COUNT;
    if (rnd(100) < (special ? RUMOUR_SPECIAL_PCT : RUMOUR_BASE_PCT)) {
      if (special) return make_special_item(wanted - BASE_COUNT, ilvl);
      Item it = make_item(wanted / TIER_COUNT, ilvl);
      it.base = (uint16_t)(wanted + 1);
      it.rarity = roll_rarity(dungeon);
      if (it.rarity >= RARITY_RARE) it.flags &= (uint8_t)~ITEM_FLAG_IDENTIFIED;
      return it;
    }
  }
  uint8_t rarity = roll_rarity(dungeon);
  if (rarity == RARITY_SET || rarity == RARITY_UNIQUE) {
    int special = pick_special(dungeon, rarity == RARITY_SET);
    if (special >= 0) return make_special_item(special, ilvl);
    rarity = RARITY_RARE;
  }
  Item it = make_item(pick_shape(DROP_SLOTS[rnd(sizeof(DROP_SLOTS))]), ilvl);
  it.rarity = rarity;
  // 黄色以上は未鑑定で落ちる
  if (rarity >= RARITY_RARE) it.flags &= (uint8_t)~ITEM_FLAG_IDENTIFIED;
  return it;
}

// ログや図鑑に出してよい base 番号（0始まり）。未鑑定の固有・セット装備は正体を隠す
static int visible_base(const Item *it) {
  if (game_item_special(it) >= 0 && !game_item_identified(it)) {
    const ShapeDef *sh = game_item_shape(it);
    return (int)(sh - g_shapes) * TIER_COUNT + game_item_tier(it);
  }
  return it->base - 1;
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

// ---- 図鑑 ----
//   0〜899: 基本アイテム（base - 1）、900〜: 固有・セット装備
int game_codex_size(void) {
  int n = BASE_COUNT + g_special_count;
  return n < CODEX_BYTES * 8 ? n : CODEX_BYTES * 8;
}

bool game_codex_found(int i) {
  if (i < 0 || i >= game_codex_size()) return false;
  return (s_codex[i / 8] >> (i % 8)) & 1;
}

CodexState game_codex_state(int i) {
  if (game_codex_found(i)) return CODEX_FOUND;
  if (i >= 0 && i < game_codex_size() && ((s_codex_seen[i / 8] >> (i % 8)) & 1)) return CODEX_SEEN;
  return CODEX_UNKNOWN;
}

// 噂で知った・取り逃した物。自分で手に入れるまでは達成率に数えない
void game_codex_mark_seen(int i) {
  if (i < 0 || i >= game_codex_size() || game_codex_state(i) != CODEX_UNKNOWN) return;
  s_codex_seen[i / 8] |= (uint8_t)(1 << (i % 8));
  s_dirty = true;
}

static void codex_set(int i) {
  if (i < 0 || i >= game_codex_size() || game_codex_found(i)) return;
  s_codex[i / 8] |= (uint8_t)(1 << (i % 8));
  game_drop_rumour(i);   // 狙っていた品なら、枠が空く
  s_dirty = true;
}

// 手に入れた物を図鑑に載せる。固有・セット装備は鑑定して名前が分かったときに載る
static void codex_mark(const Item *it) {
  if (!game_item_shape(it)) return;
  int special = game_item_special(it);
  if (special < 0) codex_set(it->base - 1);
  else if (game_item_identified(it)) codex_set(BASE_COUNT + special);
}

int game_codex_found_count(void) {
  int n = 0;
  for (int i = 0; i < CODEX_BYTES; i++) {
    for (uint8_t b = s_codex[i]; b; b &= (uint8_t)(b - 1)) n++;
  }
  return n;
}

int game_codex_seen_count(void) {
  int n = 0;
  for (int i = 0; i < CODEX_BYTES; i++) {
    for (uint8_t b = (uint8_t)(s_codex[i] | s_codex_seen[i]); b; b &= (uint8_t)(b - 1)) n++;
  }
  return n;
}

// その品が見つかる場所。基本アイテムは素材の段階から、固有・セット装備は決められたダンジョンから
void game_codex_source(int i, char *buf, size_t size) {
  buf[0] = '\0';
  if (i < 0 || i >= game_codex_size()) return;
  if (i >= BASE_COUNT) {
    const SpecialDef *d = &g_specials[i - BASE_COUNT];
    if (d->dungeon >= DUNGEON_COUNT) snprintf(buf, size, "Anywhere");
    else if (d->boss) snprintf(buf, size, "%s boss", g_dungeons[d->dungeon].name);
    else snprintf(buf, size, "%s", g_dungeons[d->dungeon].name);
    return;
  }
  // 素材の段階に合う敵のレベルのダンジョンを探す。1行に収めるため、名前は最初の1つだけ
  int tier = i % TIER_COUNT;
  int count = 0;
  const char *first = NULL;
  for (int d = 0; d < DUNGEON_COUNT; d++) {
    if (item_tier_for_level(g_dungeons[d].lvl_min) > tier ||
        item_tier_for_level(g_dungeons[d].lvl_max) < tier) {
      continue;
    }
    if (!first) first = g_dungeons[d].name;
    count++;
  }
  if (!first) snprintf(buf, size, "Unknown");
  else if (count == 1) snprintf(buf, size, "%s", first);
  else snprintf(buf, size, "%s +%d", first, count - 1);
}

// ---- 噂 ----
int game_rumour_count(void) {
  int n = 0;
  for (int i = 0; i < RUMOUR_SLOTS; i++) n += s_rumours[i] ? 1 : 0;
  return n;
}

int game_rumour_at(int slot) {
  if (slot < 0 || slot >= RUMOUR_SLOTS || !s_rumours[slot]) return -1;
  return s_rumours[slot] - 1;
}

bool game_rumour_has(int entry) {
  if (entry < 0) return false;
  for (int i = 0; i < RUMOUR_SLOTS; i++) {
    if (s_rumours[i] == entry + 1) return true;
  }
  return false;
}

// 狙いを定める料金。素材の段階が高いほど、固有・セット装備はさらに高い
int game_rumour_price(int entry) {
  if (entry < 0 || entry >= game_codex_size() || game_codex_found(entry)) return 0;
  if (entry >= BASE_COUNT) return 5000;
  return 500 + 300 * (entry % TIER_COUNT);
}

RumourResult game_buy_rumour(int entry) {
  int price = game_rumour_price(entry);
  if (!price || game_rumour_has(entry)) return RUMOUR_NONE;
  if (game_rumour_count() >= RUMOUR_SLOTS) return RUMOUR_FULL;
  if (s_hero.gold < price) return RUMOUR_NO_GOLD;
  for (int i = 0; i < RUMOUR_SLOTS; i++) {
    if (s_rumours[i]) continue;
    s_hero.gold -= price;
    s_rumours[i] = (uint16_t)(entry + 1);
    game_codex_mark_seen(entry);   // 噂を聞いた時点で、姿は分かる
    game_save();
    return RUMOUR_OK;
  }
  return RUMOUR_FULL;
}

void game_drop_rumour(int entry) {
  for (int i = 0; i < RUMOUR_SLOTS; i++) {
    if (s_rumours[i] != entry + 1) continue;
    s_rumours[i] = 0;
    s_dirty = true;
  }
}

void game_codex_name(int i, char *buf, size_t size) {
  Item probe = { 0 };
  probe.base = (uint16_t)(i + 1);
  probe.flags = ITEM_FLAG_IDENTIFIED;
  probe.ilvl = 1;
  game_item_short_name(&probe, buf, size);
}

int game_codex_rarity(int i) {
  if (i < BASE_COUNT) return RARITY_NORMAL;
  return g_specials[i - BASE_COUNT].set_id ? RARITY_SET : RARITY_UNIQUE;
}

static bool bag_add(const Item *it) {
  int n = game_bag_count();
  if (n >= BAG_SIZE) return false;
  s_bag[n] = *it;
  codex_mark(it);
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

// その品を身に着けるとき、どの部位の物と入れ替わるか（指輪は空いている方へ）
static int equip_target_slot(const ShapeDef *sh) {
  int slot = sh->slot;
  if (slot == SLOT_RING1 && s_equip[SLOT_RING1].base && !s_equip[SLOT_RING2].base) slot = SLOT_RING2;
  return slot;
}

// ざっくりした強さ（ATK と DEF は HP より重く見る）
static int stat_score(ItemStats s) { return s.atk * 3 + s.def * 3 + s.hp; }

Compare game_item_compare(const Item *it, const Item **now, ItemStats *diff) {
  if (now) *now = NULL;
  if (diff) memset(diff, 0, sizeof(*diff));
  const ShapeDef *sh = game_item_shape(it);
  if (!sh || !game_item_identified(it)) return CMP_NONE;
  const Item *cur = game_equipped(equip_target_slot(sh));
  if (cur == it) return CMP_NONE;   // 今つけている物そのもの
  ItemStats a = game_item_stats(it);
  ItemStats b = { 0, 0, 0 };
  if (cur) b = game_item_stats(cur);
  if (now) *now = cur;
  if (diff) {
    diff->atk = (int16_t)(a.atk - b.atk);
    diff->def = (int16_t)(a.def - b.def);
    diff->hp = (int16_t)(a.hp - b.hp);
  }
  int d = stat_score(a) - stat_score(b);
  return d > 0 ? CMP_BETTER : (d < 0 ? CMP_WORSE : CMP_EVEN);
}

bool game_equip_from_bag(int bag_index) {
  const Item *it = game_bag(bag_index);
  const ShapeDef *sh = game_item_shape(it);
  if (!sh) return false;
  if (!game_item_identified(it)) return false;   // 未鑑定の物は身に着けられない
  int slot = equip_target_slot(sh);
  Item picked = *it;
  Item old = s_equip[slot];
  s_equip[slot] = picked;
  if (old.base) s_bag[bag_index] = old;   // 外した物は同じ場所へ
  else bag_remove(bag_index);
  game_save();
  return true;
}

bool game_unequip(int slot) {
  if (!game_equipped(slot)) return false;
  if (!bag_add(&s_equip[slot])) return false;
  memset(&s_equip[slot], 0, sizeof(Item));
  game_save();
  return true;
}

bool game_sell_bag(int bag_index) {
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
static int with_pct(int value, Effect fx) {
  return value * (100 + game_effect_total(fx) + game_effect_total(FX_ALL)) / 100;
}

int game_max_hp(void) { return with_pct(50 + 12 * s_hero.level + gear_total().hp, FX_HP); }
int game_atk(void) { return with_pct(4 + 2 * s_hero.level + gear_total().atk, FX_ATK); }
int game_def(void) { return with_pct(s_hero.level + gear_total().def, FX_DEF); }
int game_potions(void) { return s_hero.potions; }
int game_portals(void) { return s_hero.portals; }

int game_hp(void) {
  return s_run.mode == RUN_NONE ? game_max_hp() : s_run.hp;
}

// ============================================================
// 設定
// ============================================================
int game_auto_return_pct(void) { return s_hero.auto_return_pct; }

int game_remind_hour(void) { return s_hero.remind_hour < 24 ? s_hero.remind_hour : 0; }

void game_cycle_remind(void) {
  static const uint8_t CHOICES[] = { 0, 6, 7, 8, 9, 10, 12 };
  int n = sizeof(CHOICES);
  int cur = 0;
  for (int i = 0; i < n; i++) {
    if (CHOICES[i] == s_hero.remind_hour) cur = i;
  }
  s_hero.remind_hour = CHOICES[(cur + 1) % n];
  game_save();
}

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
int game_run_position(void) { return (int)position(); }
int game_run_total_steps(void) { return (int)total_steps(cur_dungeon()); }
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
    heal(max / 2 * (100 + game_effect_total(FX_POTION)) / 100);
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

// 自動帰還の判定
//   奥へ進んでいるとき: 巻物があれば使い、なければ歩いて帰り始める
//   帰り道: 弱ったまま歩き続けると死ぬので、巻物があればここでも使う
static void check_auto_return(void) {
  if (s_run.mode == RUN_NONE || !s_hero.auto_return_pct) return;
  if (s_run.hp * 100 >= game_max_hp() * s_hero.auto_return_pct) return;
  if (s_hero.portals > 0) {
    s_alert = true;
    s_hero.portals--;
    log_push(LOG_PORTAL, 0, 0, 0, 0);
    arrive_town();
    return;
  }
  if (s_run.mode == RUN_EXPLORE) {
    s_alert = true;
    log_push(LOG_LOW_HP, 0, 0, 0, 0);
    start_return();
  } else if (!s_low_hp_alerted) {
    // 帰り道で巻物がないときは、知らせるのは最初の一度だけ
    s_alert = true;
    s_low_hp_alerted = true;
    log_push(LOG_LOW_HP, 0, 0, 0, 0);
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
  Item it = make_drop_item(ilvl, s_run.dungeon);
  if (bag_add(&it)) {
    s_batch.items++;
    log_push(LOG_ITEM, it.rarity, visible_base(&it), 0, 0);
  } else {
    game_codex_mark_seen(visible_base(&it));   // 取り逃した物も、見たことにはなる
    log_push(LOG_BAG_FULL, it.rarity, visible_base(&it), 0, 0);
  }
}

// ボスを倒したときの戦利品。一定の確率で、そのボスだけが落とす固有装備
#define BOSS_UNIQUE_CHANCE 15

static void found_boss_loot(int dungeon, int ilvl) {
  // そのダンジョンのボス専用ユニークを狙っていれば出やすく、空振りが続けば必ず出す
  int boss_entry = -1;
  for (int i = 0; i < g_special_count; i++) {
    if (g_specials[i].boss && g_specials[i].dungeon == dungeon) {
      boss_entry = BASE_COUNT + i;
      break;
    }
  }
  int chance = BOSS_UNIQUE_CHANCE;
  bool sure = false;
  if (boss_entry >= 0 && !game_codex_found(boss_entry)) {
    if (game_rumour_has(boss_entry)) chance = RUMOUR_BOSS_PCT;
    if (s_boss_miss[dungeon] + 1 >= BOSS_PITY) sure = true;
  }
  if (sure || rnd(100) < chance) {
    s_boss_miss[dungeon] = 0;
    s_dirty = true;
    for (int i = 0; i < g_special_count; i++) {
      if (!g_specials[i].boss || g_specials[i].dungeon != dungeon) continue;
      Item it = make_special_item(i, ilvl);
      if (bag_add(&it)) {
        s_batch.items++;
        log_push(LOG_ITEM, it.rarity, visible_base(&it), 0, 0);
      } else {
        game_codex_mark_seen(visible_base(&it));
        log_push(LOG_BAG_FULL, it.rarity, visible_base(&it), 0, 0);
      }
      return;
    }
  }
  if (boss_entry >= 0 && !game_codex_found(boss_entry) && s_boss_miss[dungeon] < 255) {
    s_boss_miss[dungeon]++;
    s_dirty = true;
  }
  found_item(ilvl);
}

static void add_gold(int g) {
  s_hero.gold += g;
  s_run.run_gold += g;
  s_batch.gold += g;
}

// ダンジョンで見つけたお金（ゴールド発見の効果がつく）
static int loot_gold(int g) {
  g = g * (100 + game_effect_total(FX_GOLD) + sleep_bonus(FX_GOLD)) / 100;
  add_gold(g);
  return g;
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
  int guard = game_effect_total(FX_GUARD);
  for (int round = 0; round < 60 && mhp > 0; round++) {
    mhp -= damage(atk, mdef);
    if (mhp <= 0) break;
    int hit = damage(matk, def) * (100 - guard) / 100;
    s_run.hp -= hit < 1 ? 1 : hit;
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
  xp = xp * (100 + game_effect_total(FX_XP) + sleep_bonus(FX_XP)) / 100;
  gold = loot_gold(gold);
  // 吸収: 勝つと最大HPの一部を回復
  int leech = game_effect_total(FX_LEECH);
  if (leech) heal(game_max_hp() * leech / 100);
  s_batch.battles++;
  log_push(boss ? LOG_BOSS : LOG_BATTLE, monster, taken, xp, gold);
  add_xp(xp);
  return true;
}

// ダメージを受けた後の処理。死んだら true
static bool hurt(int d) {
  d = d * (100 - game_effect_total(FX_TRAP)) / 100;
  if (d < 1) d = 1;
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
    g = loot_gold(g);
    log_push(LOG_GOLD, 0, g, 0, 0);
  } else if (r < 85) {
    int d = game_max_hp() * (8 + rnd(8)) / 100;
    if (!hurt(d < 1 ? 1 : d)) check_auto_return();
  } else if (r < 93) {
    fountain();
  }
  // 残りは何も起きない
}

// 帰り道の出来事（奥へ進むときより穏やか。拾い物もあるが確率は低い）
static void return_event(void) {
  int mlvl = monster_level(floor_at(s_run.return_left));
  int r = rnd(100);
  if (r < 35) {
    if (battle(s_run.dungeon * 4 + rnd(3), mlvl, false)) {
      if (rnd(100) < 6) found_item(mlvl);
      check_auto_return();
    }
  } else if (r < 45) {
    int g = 2 + mlvl * 2 + rnd(mlvl * 2 + 2);
    g = loot_gold(g);
    log_push(LOG_GOLD, 0, g, 0, 0);
  } else if (r < 52) {
    int d = game_max_hp() * (6 + rnd(6)) / 100;
    if (!hurt(d < 1 ? 1 : d)) check_auto_return();
  } else if (r < 58) {
    fountain();
  } else if (r < 62) {
    found_item(mlvl);
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

// 歩数 n 歩分、冒険を進める。町に着いたり死んだりして使い切れなかった歩数を返す
static int32_t advance(int32_t n) {
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
          found_boss_loot(s_run.dungeon, d->lvl_max);
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
  return n;
}

// 歩数 raw 歩を冒険に使う（ストライドで多めに進む）。使い切れなかった歩数を、元の歩数に直して返す
static int32_t walk_steps(int32_t raw) {
  if (raw <= 0) return 0;
  int pct = 100 + game_effect_total(FX_STRIDE);
  memset(&s_batch, 0, sizeof(s_batch));
  int32_t left = advance(raw * pct / 100);
  // 一度にたくさん起きたとき（アプリを閉じていた間など）は、まとめを一番新しいログに出す
  if (s_batch.events > LOG_SUMMARY_MIN) {
    log_push(LOG_SUMMARY, s_batch.levels, s_batch.battles, s_batch.gold, s_batch.items);
  }
  return left * 100 / pct;
}

// 町にいる間の歩数を貯める（上限あり）
static void add_saved_steps(int32_t n) {
  if (n <= 0) return;
  int32_t total = s_hero.saved_steps + n;
  s_hero.saved_steps = total < SAVED_STEPS_MAX ? total : SAVED_STEPS_MAX;
  s_dirty = true;
}

int32_t game_saved_steps(void) { return s_hero.saved_steps; }

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
  s_low_hp_alerted = false;
  log_push(LOG_DEPART, idx, 0, 0, 0);
  if (s_hero.saved_steps > 0) {
    int32_t use = s_hero.saved_steps;
    s_hero.saved_steps = 0;
    log_push(LOG_SAVED_STEPS, 0, use, 0, 0);
    int32_t left = walk_steps(use);
    if (s_run.mode == RUN_NONE) add_saved_steps(left);   // 使い切る前に帰ってきたら、残りはまた貯める
  }
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
// 装備枠ごとに1つ。格0〜1の形から、勇者のレベルごとに決まった物を並べる
Item game_shop_gear(int i) {
  int slot = DROP_SLOTS[i % SHOP_GEAR_COUNT];
  int count = 0;
  for (int s = 0; s < SHAPE_COUNT; s++) count += (g_shapes[s].slot == slot && g_shapes[s].rank <= 1);
  int pick = (s_hero.level * 7 + i * 13) % (count > 0 ? count : 1);
  int shape = SH_SHORT_SWORD;
  for (int s = 0; s < SHAPE_COUNT; s++) {
    if (g_shapes[s].slot != slot || g_shapes[s].rank > 1) continue;
    if (pick-- == 0) {
      shape = s;
      break;
    }
  }
  Item it = { 0 };
  int lv = s_hero.level;
  it.base = (uint16_t)(shape * TIER_COUNT + item_tier_for_level(lv) + 1);
  it.ilvl = (uint8_t)lv;
  it.seed = (uint16_t)((lv * 131 + i * 17) & 0xFFFF);
  it.rarity = RARITY_NORMAL;
  it.flags = ITEM_FLAG_IDENTIFIED;
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

BuyResult game_buy_identify(void) {
  if (s_hero.scrolls_id >= MAX_IDENT_SCROLLS) return BUY_FULL;
  if (s_hero.gold < IDENT_SCROLL_PRICE) return BUY_NO_GOLD;
  s_hero.gold -= IDENT_SCROLL_PRICE;
  s_hero.scrolls_id++;
  game_save();
  return BUY_OK;
}

// ============================================================
// 鑑定
// ============================================================
int game_identify_scrolls(void) { return s_hero.scrolls_id; }

int game_identify_fee(const Item *it) {
  if (!it || !game_item_shape(it)) return 0;
  // 良い物ほど高い
  int fee = 20 + it->ilvl * 4;
  if (it->rarity >= RARITY_SET) fee = fee * 3 / 2;
  return fee;
}

int game_unidentified_count(void) {
  int n = 0;
  for (int i = 0; i < BAG_SIZE; i++) {
    if (s_bag[i].base && !game_item_identified(&s_bag[i])) n++;
  }
  return n;
}

static IdentResult identify(int bag_index) {
  const Item *it = game_bag(bag_index);
  if (!it || game_item_identified(it)) return IDENT_NONE;
  s_bag[bag_index].flags |= ITEM_FLAG_IDENTIFIED;
  codex_mark(&s_bag[bag_index]);   // セット・固有装備はここで図鑑に載る
  log_push(LOG_IDENTIFY, s_bag[bag_index].rarity, s_bag[bag_index].base - 1, 0, 0);
  game_save();
  return IDENT_OK;
}

IdentResult game_identify_with_gold(int bag_index) {
  const Item *it = game_bag(bag_index);
  if (!it || game_item_identified(it)) return IDENT_NONE;
  if (s_run.mode != RUN_NONE) return IDENT_NONE;   // 鑑定屋は町にしかいない
  int fee = game_identify_fee(it);
  if (s_hero.gold < fee) return IDENT_NO_GOLD;
  s_hero.gold -= fee;
  return identify(bag_index);
}

IdentResult game_identify_with_scroll(int bag_index) {
  const Item *it = game_bag(bag_index);
  if (!it || game_item_identified(it)) return IDENT_NONE;
  if (s_hero.scrolls_id == 0) return IDENT_NO_SCROLL;
  s_hero.scrolls_id--;
  return identify(bag_index);
}

// ============================================================
// 保管庫
// ============================================================
int game_stash_count(void) {
  int n = 0;
  while (n < STASH_SIZE && s_stash[n].base) n++;
  return n;
}

const Item *game_stash(int i) {
  return (i >= 0 && i < STASH_SIZE && s_stash[i].base) ? &s_stash[i] : NULL;
}

bool game_stash_put(int bag_index) {
  if (s_run.mode != RUN_NONE) return false;
  const Item *it = game_bag(bag_index);
  int n = game_stash_count();
  if (!it || n >= STASH_SIZE) return false;
  s_stash[n] = *it;
  bag_remove(bag_index);
  game_save();
  return true;
}

bool game_stash_take(int stash_index) {
  if (s_run.mode != RUN_NONE) return false;
  int n = game_stash_count();
  if (stash_index < 0 || stash_index >= n || game_bag_count() >= BAG_SIZE) return false;
  bag_add(&s_stash[stash_index]);
  memmove(&s_stash[stash_index], &s_stash[stash_index + 1], sizeof(Item) * (n - stash_index - 1));
  memset(&s_stash[n - 1], 0, sizeof(Item));
  game_save();
  return true;
}

// ============================================================
// 鍛冶屋
// ============================================================
// 今の段階から次の段階へ上がる確率（+1〜+3 は必ず成功）
static const uint8_t UPGRADE_CHANCE[MAX_PLUS] = { 100, 100, 100, 90, 80, 70, 60, 50, 40, 30 };

int game_upgrade_cost(const Item *it) {
  if (!game_item_shape(it) || !game_item_identified(it) || it->plus >= MAX_PLUS) return 0;
  int p = it->plus;
  return (25 + it->ilvl * 5) * (p + 1) * (p + 2) / 4;
}

int game_upgrade_chance(const Item *it) {
  if (!it || it->plus >= MAX_PLUS) return 0;
  return UPGRADE_CHANCE[it->plus];
}

static UpgradeResult upgrade(Item *it) {
  if (s_run.mode != RUN_NONE || !game_item_shape(it)) return UPGRADE_NONE;
  if (it->plus >= MAX_PLUS) return UPGRADE_MAX;
  int cost = game_upgrade_cost(it);
  if (cost == 0) return UPGRADE_NONE;   // 未鑑定
  if (s_hero.gold < cost) return UPGRADE_NO_GOLD;
  s_hero.gold -= cost;
  // 失敗してもお金が減るだけ。装備は壊れない
  bool ok = rnd(100) < UPGRADE_CHANCE[it->plus];
  if (ok) it->plus++;
  log_push(LOG_UPGRADE, ok ? it->plus : it->plus + 1, it->base - 1, ok ? 1 : 0, 0);
  game_save();
  return ok ? UPGRADE_OK : UPGRADE_FAILED;
}

UpgradeResult game_upgrade_equipped(int slot) {
  if (slot < 0 || slot >= EQUIP_SLOTS || !s_equip[slot].base) return UPGRADE_NONE;
  return upgrade(&s_equip[slot]);
}

UpgradeResult game_upgrade_bag(int bag_index) {
  if (!game_bag(bag_index)) return UPGRADE_NONE;
  return upgrade(&s_bag[bag_index]);
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
  bool slept = check_sleep();   // ボーナスは、これから反映する歩数にも効く
  check_drop_expire();
  if (s_run.mode != RUN_NONE) {
    if (!steps_available()) {
      if (!s_warned_no_steps) {
        s_warned_no_steps = true;
        log_push(LOG_NO_STEPS, 0, 0, 0, 0);
      }
    } else {
      int32_t left = walk_steps(steps_since(&s_run.snap));
      // 途中で町に着いた（または倒れた）なら、残りの歩数は次の出発のために貯める
      if (s_run.mode == RUN_NONE) add_saved_steps(left);
    }
  } else if (steps_available()) {
    // 町にいる間の歩数も無駄にしない（前回の記録は探索の最後の記録がそのまま続く）
    int32_t n = steps_since(&s_run.snap);
    if (n > 0) {
      add_saved_steps(n);
      s_dirty = true;
    }
  }
  // まとめのログに埋もれないよう、睡眠のログは最後に出す
  if (slept) log_push(LOG_SLEEP, s_hero.sleep_tier, s_hero.sleep_min, 0, 0);
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
  persist_write_data(KEY_STASH_A, s_stash, sizeof(Item) * (STASH_SIZE / 2));
  persist_write_data(KEY_STASH_B, &s_stash[STASH_SIZE / 2], sizeof(Item) * (STASH_SIZE / 2));
  persist_write_data(KEY_CODEX, s_codex, sizeof(s_codex));
  persist_write_data(KEY_CODEX_SEEN, s_codex_seen, sizeof(s_codex_seen));
  persist_write_data(KEY_RUMOUR, s_rumours, sizeof(s_rumours));
  persist_write_data(KEY_BOSS_MISS, s_boss_miss, sizeof(s_boss_miss));
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
  memset(s_stash, 0, sizeof(s_stash));
  memset(s_codex, 0, sizeof(s_codex));
  memset(s_codex_seen, 0, sizeof(s_codex_seen));
  memset(s_rumours, 0, sizeof(s_rumours));
  memset(s_boss_miss, 0, sizeof(s_boss_miss));
  s_hero.version = SAVE_VERSION;
  s_hero.level = 1;
  s_hero.gold = 100;
  s_hero.potions = 3;
  s_hero.portals = 1;
  s_hero.auto_return_pct = 30;
  s_hero.remind_hour = 8;
  steps_snapshot(&s_run.snap);   // 始めた時点より前の歩数は数えない
  s_run.rng = (uint32_t)time(NULL);
  // 最初の装備
  s_equip[SLOT_WEAPON] = make_item(SH_SHORT_SWORD, 1);
  s_equip[SLOT_BODY] = make_item(SH_TUNIC, 1);
  codex_mark(&s_equip[SLOT_WEAPON]);
  codex_mark(&s_equip[SLOT_BODY]);
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
  memset(s_stash, 0, sizeof(s_stash));
  memset(s_codex, 0, sizeof(s_codex));
  memset(s_codex_seen, 0, sizeof(s_codex_seen));
  memset(s_rumours, 0, sizeof(s_rumours));
  memset(s_boss_miss, 0, sizeof(s_boss_miss));
  persist_read_data(KEY_STASH_A, s_stash, sizeof(Item) * (STASH_SIZE / 2));
  persist_read_data(KEY_STASH_B, &s_stash[STASH_SIZE / 2], sizeof(Item) * (STASH_SIZE / 2));
  bool had_codex = persist_read_data(KEY_CODEX, s_codex, sizeof(s_codex)) > 0;
  persist_read_data(KEY_CODEX_SEEN, s_codex_seen, sizeof(s_codex_seen));
  persist_read_data(KEY_RUMOUR, s_rumours, sizeof(s_rumours));
  persist_read_data(KEY_BOSS_MISS, s_boss_miss, sizeof(s_boss_miss));
  if (!had_codex) {
    // 図鑑ができる前のセーブ: 今持っている物から図鑑を作る
    for (int i = 0; i < EQUIP_SLOTS; i++) codex_mark(&s_equip[i]);
    for (int i = 0; i < BAG_SIZE; i++) codex_mark(&s_bag[i]);
    for (int i = 0; i < STASH_SIZE; i++) codex_mark(&s_stash[i]);
  }
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
