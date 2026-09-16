#pragma once
#include <pebble.h>

// ============================================================
// ゲームデータとロジック（画面から独立した部分）
//   ・ダンジョンの進行は実際の歩数に連動する（steps.c）
//   ・出来事は番号で記録し、表示するときに文章にする（game_log_text）
// ============================================================

#define DUNGEON_COUNT 8
#define EQUIP_SLOTS 9
#define BAG_SIZE 24
#define LOG_SIZE 24
#define LOG_IMPORTANT_MAX 12   // 重要な出来事はこの件数まで、普通の出来事に押し出されない
#define LOG_SUMMARY_MIN 6      // 1回の更新でこれより多く起きたら、まとめのログを出す
#define MAX_LEVEL 50
#define MAX_POTIONS 5
#define MAX_PORTALS 9
#define DROP_EXPIRE_SEC (72 * 60 * 60)   // 死亡時に残した物が消えるまで
#define POTION_PRICE 25
#define PORTAL_PRICE 60
#define MAX_IDENT_SCROLLS 9
#define IDENT_SCROLL_PRICE 45

// ---- 装備枠 ----
typedef enum {
  SLOT_WEAPON, SLOT_OFFHAND, SLOT_HEAD, SLOT_BODY, SLOT_HANDS, SLOT_FEET,
  SLOT_AMULET, SLOT_RING1, SLOT_RING2,
} EquipSlot;

// ---- レア度 ----
typedef enum {
  RARITY_NORMAL = 0,   // 白：接辞なし
  RARITY_MAGIC,        // 青：接辞 1〜2
  RARITY_RARE,         // 黄：接辞 3〜4
  RARITY_SET,          // 緑：セット装備
  RARITY_UNIQUE,       // 金：固有装備
  RARITY_COUNT,
} Rarity;

#define ITEM_FLAG_IDENTIFIED 0x01
// セット・固有装備の番号（g_specials の添字）を flags の上位ビットに入れる
#define ITEM_SPECIAL_SHIFT 1
#define ITEM_SPECIAL_MASK 0x7E
#define MAX_AFFIXES 4
#define MAX_PLUS 10

// ---- アイテム（保存形式。1個8バイト） ----
//   性能は base・seed・ilvl から毎回計算する（保存領域が小さいため）
typedef struct {
  uint16_t base;     // 基本アイテムの番号 + 1（0 は空）
  uint16_t seed;     // 接辞と性能のばらつきの元
  uint8_t ilvl;      // アイテムレベル
  uint8_t rarity;    // Rarity
  uint8_t flags;     // 鑑定済み + セット/固有装備の番号
  uint8_t plus;      // 強化段階（0〜MAX_PLUS）
} Item;

typedef struct {
  const char *name;
  uint8_t slot;      // EquipSlot（指輪は SLOT_RING1）
  uint8_t icon;      // items.png の番号
} BaseDef;

// ---- 接辞（seed から決まる。名前と性能の両方に効く） ----
typedef enum { AFFIX_ATK, AFFIX_DEF, AFFIX_HP } AffixStat;

typedef struct {
  const char *name;
  uint8_t stat;      // AffixStat
  uint8_t power;     // 強さ（100 = 標準）
} AffixDef;

// ---- セット・固有装備 ----
typedef struct {
  const char *name;
  uint8_t base;      // 基本アイテムの番号
  uint8_t bonus_pct; // 基本性能の倍率（%）
  int16_t atk, def, hp;   // 追加の性能（アイテムレベルによらない）
  uint8_t set_id;    // 0 = 固有装備、1以上 = セット番号
} SpecialDef;

typedef struct {
  int16_t atk;
  int16_t def;
  int16_t hp;
} ItemStats;

typedef struct {
  const char *name;
  uint8_t floors;
  uint16_t steps_per_floor;
  uint8_t lvl_min, lvl_max;   // 敵のレベル（1階 → 最深部）
  uint8_t art;                // 背景画像の番号
  const char *monsters[4];    // 3種 + ボス
} DungeonDef;

extern const DungeonDef g_dungeons[DUNGEON_COUNT];
extern const BaseDef g_bases[];
extern const int g_base_count;
extern const char *const g_slot_names[EQUIP_SLOTS];

// ---- 冒険の状態 ----
typedef enum {
  RUN_NONE = 0,     // 町にいる
  RUN_EXPLORE = 1,  // 奥へ進んでいる
  RUN_RETURN = 2,   // 歩いて町へ戻っている
} RunMode;

// ---- ログ ----
typedef enum {
  LOG_NONE,
  LOG_WELCOME,
  LOG_DEPART,       // a=ダンジョン
  LOG_FLOOR,        // a=階
  LOG_BATTLE,       // a=敵 b=受けたダメージ c=経験値 d=ゴールド
  LOG_BOSS,         // a=敵 b=受けたダメージ c=経験値 d=ゴールド
  LOG_ITEM,         // b=基本アイテム
  LOG_BAG_FULL,     // b=基本アイテム
  LOG_GOLD,         // b=量
  LOG_TRAP,         // b=ダメージ
  LOG_FOUNTAIN,     // b=回復量
  LOG_POTION,       // b=回復量
  LOG_LEVEL,        // a=レベル
  LOG_LOW_HP,       // 自動帰還の開始（巻物なし）
  LOG_PORTAL,       // 帰還の巻物を使った
  LOG_WALK_BACK,    // 自分で歩いて帰ることにした
  LOG_BOTTOM,       // 最深部を制覇して帰り始めた
  LOG_TOWN,         // 町に着いた
  LOG_DEATH,        // a=ダンジョン b=階
  LOG_RECOVER,      // b=回収した数 c=残った数
  LOG_DROP_GONE,    // 残した物が消えた
  LOG_AGENT,        // b=回収した数（代行業者）
  LOG_NO_STEPS,     // 歩数が取れない
  LOG_SUMMARY,      // まとめ a=レベル b=戦闘 c=ゴールド d=アイテム
  LOG_IDENTIFY,     // 鑑定した a=レア度 b=基本アイテム
} LogType;

// レア度の呼び名（White/Blue/Yellow/Green/Gold）
extern const char *const g_rarity_names[RARITY_COUNT];

// ログの見た目の種類（画面で色を変える）
typedef enum {
  LOG_STYLE_DUNGEON,  // ダンジョンでの出来事
  LOG_STYLE_TOWN,     // 町での出来事
  LOG_STYLE_GREAT,    // ボス撃破・レベルアップ・回収など
  LOG_STYLE_DANGER,   // 死亡・消失・HP低下
  LOG_STYLE_SUMMARY,  // まとめ
} LogStyle;

typedef struct {
  uint8_t type;
  uint8_t a;
  uint16_t b;
  uint16_t c;
  uint16_t d;
} LogEntry;

void game_init(void);
void game_save(void);

// 歩数を確認して冒険を進める。何か起きたら true
bool game_update(void);

// ---- 勇者 ----
int game_level(void);
int32_t game_xp(void);
int32_t game_xp_next(void);
int32_t game_gold(void);
int game_hp(void);
int game_max_hp(void);
int game_atk(void);
int game_def(void);
int game_potions(void);
int game_portals(void);

// ---- 冒険 ----
RunMode game_run_mode(void);
int game_run_dungeon(void);
int game_floor(void);              // 今いる階（1〜）
int game_steps_to_next_floor(void);
int game_return_left(void);
bool game_dungeon_unlocked(int idx);
bool game_dungeon_cleared(int idx);
bool game_depart(int idx);
bool game_use_portal(void);
void game_walk_back(void);
bool game_steps_available(void);

// ---- 設定 ----
int game_auto_return_pct(void);    // 0 = しない
void game_cycle_auto_return(void);
bool game_vibrate(void);
void game_toggle_vibrate(void);

// ---- アイテム ----
extern const AffixDef g_prefixes[];
extern const int g_prefix_count;
extern const AffixDef g_suffixes[];
extern const int g_suffix_count;
extern const SpecialDef g_specials[];
extern const int g_special_count;

// 未鑑定なら基本性能だけ、鑑定済みなら接辞と強化も含めた性能
ItemStats game_item_stats(const Item *it);
const BaseDef *game_item_base(const Item *it);
int game_item_price(const Item *it);        // 売値
// 接辞・セット・固有装備を含めた名前（未鑑定なら基本アイテム名のまま）
void game_item_name(const Item *it, char *buf, size_t size);
bool game_item_identified(const Item *it);
int game_item_special(const Item *it);      // g_specials の添字。なければ -1
// 鑑定済みの接辞の数と、i 番目の接辞（未鑑定なら 0 個）
int game_item_affix_count(const Item *it);
const AffixDef *game_item_affix(const Item *it, int i, int *value);
// 同じセットの装備を何個つけているか（セット装備でなければ 0）
int game_set_pieces_equipped(const Item *it);

const Item *game_equipped(int slot);
const Item *game_bag(int i);
int game_bag_count(void);
bool game_can_change_gear(void);
bool game_equip_from_bag(int bag_index);
bool game_unequip(int slot);
bool game_sell_bag(int bag_index);

// ---- お店 ----
#define SHOP_GEAR_COUNT 8
Item game_shop_gear(int i);                 // 今の在庫（勇者のレベルに合わせる）
int game_shop_gear_cost(int i);
typedef enum { BUY_OK, BUY_NO_GOLD, BUY_FULL } BuyResult;
BuyResult game_buy_gear(int i);
BuyResult game_buy_potion(void);
BuyResult game_buy_portal(void);
BuyResult game_buy_identify(void);

// ---- 鑑定 ----
//   黄色以上は未鑑定で落ちる。町の鑑定屋（お金）か、鑑定の巻物（ダンジョンでも使える）
int game_identify_scrolls(void);
int game_identify_fee(const Item *it);
int game_unidentified_count(void);
typedef enum { IDENT_OK, IDENT_NO_GOLD, IDENT_NO_SCROLL, IDENT_NONE } IdentResult;
IdentResult game_identify_with_gold(int bag_index);
IdentResult game_identify_with_scroll(int bag_index);

// ---- 死亡時に残した物 ----
bool game_drop_exists(void);
int game_drop_dungeon(void);
int game_drop_floor(void);
int game_drop_item_count(void);
int32_t game_drop_seconds_left(void);
int32_t game_drop_fee(void);
typedef enum { AGENT_OK, AGENT_NO_GOLD, AGENT_NO_ROOM, AGENT_NONE } AgentResult;
// 依頼できるかどうかだけを調べる（お金は減らない）
AgentResult game_agent_check(void);
AgentResult game_hire_agent(void);

// ---- ログ ----
int game_log_count(void);
// 新しい順に i 番目のログを文章にする
void game_log_text(int i, char *buf, size_t size);
LogStyle game_log_style(int i);
