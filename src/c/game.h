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
#define SLEEP_RESTED_SEC (6 * 60 * 60)      // これ以上眠ると Rested
#define SLEEP_REFRESHED_SEC (7 * 60 * 60)   // これ以上眠ると Refreshed
#define SAVED_STEPS_MAX 6000                  // 町にいる間に貯めておける歩数
#define DROP_EXPIRE_SEC (72 * 60 * 60)   // 死亡時に残した物が消えるまで
#define POTION_PRICE 25
#define PORTAL_PRICE 60
#define MAX_IDENT_SCROLLS 9
#define IDENT_SCROLL_PRICE 45
#define STASH_SIZE 48          // 保管庫（24個ずつ2つのキーに保存）
#define CODEX_BYTES 128        // 図鑑の既読ビット（1024種類まで）

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
#define MAX_AFFIXES 4
#define MAX_PLUS 10

// ---- アイテム（保存形式。1個8バイト） ----
//   性能は base・seed・ilvl から毎回計算する（保存領域が小さいため）
typedef struct {
  uint16_t base;     // 1〜900: 形×素材、901〜: 固有・セット装備（0 は空）。items.h 参照
  uint16_t seed;     // 接辞と性能のばらつきの元
  uint8_t ilvl;      // アイテムレベル
  uint8_t rarity;    // Rarity
  uint8_t flags;     // ITEM_FLAG_IDENTIFIED
  uint8_t plus;      // 強化段階（0〜MAX_PLUS）
} Item;

#include "items.h"

// ---- 接辞（seed から決まる。名前と性能の両方に効く） ----
typedef enum { AFFIX_ATK, AFFIX_DEF, AFFIX_HP } AffixStat;

typedef struct {
  const char *name;
  uint8_t stat;      // AffixStat
  uint8_t power;     // 強さ（100 = 標準）
} AffixDef;

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
  LOG_UPGRADE,      // 鍛冶屋 a=強化後の段階 b=基本アイテム c=成功なら1
  LOG_SAVED_STEPS,       // 出発時に貯めた歩数を使った b=歩数
  LOG_SLEEP,        // 睡眠のボーナス a=段階 b=眠った分数
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
// ダンジョンの中での位置（入口からの歩数）と、最深部までの歩数
int game_run_position(void);
int game_run_total_steps(void);
bool game_dungeon_unlocked(int idx);
bool game_dungeon_cleared(int idx);
bool game_depart(int idx);
bool game_use_portal(void);
void game_walk_back(void);
bool game_steps_available(void);
// ---- 睡眠のボーナス（昨夜よく眠れたら、その日は経験値・ゴールドなどが増える） ----
typedef enum { SLEEP_NONE, SLEEP_RESTED, SLEEP_REFRESHED } SleepTier;
SleepTier game_sleep_tier(void);
int game_sleep_minutes(void);
int32_t game_saved_steps(void);   // 町にいる間に貯めた歩数（次の出発で使う）

// ---- 設定 ----
int game_auto_return_pct(void);    // 0 = しない
void game_cycle_auto_return(void);
bool game_vibrate(void);
void game_toggle_vibrate(void);
int game_remind_hour(void);        // 0 = 知らせない。町にいたらこの時刻にアプリを起こす
void game_cycle_remind(void);

// ---- アイテム ----
extern const AffixDef g_prefixes[];
extern const int g_prefix_count;
extern const AffixDef g_suffixes[];
extern const int g_suffix_count;

// 未鑑定なら基本性能だけ、鑑定済みなら接辞と強化も含めた性能
ItemStats game_item_stats(const Item *it);
const ShapeDef *game_item_shape(const Item *it);   // 空なら NULL
int game_item_tier(const Item *it);                // 素材の段階 0〜5
int game_item_price(const Item *it);               // 売値
// 一覧用の短い名前: 「素材 + 形」か、鑑定済みの固有・セット装備の名前
void game_item_short_name(const Item *it, char *buf, size_t size);
// 詳細画面用の長い名前: 強化段階と接辞も含む（未鑑定なら短い名前と同じ）
void game_item_name(const Item *it, char *buf, size_t size);
bool game_item_identified(const Item *it);
int game_item_special(const Item *it);      // g_specials の添字。なければ -1
// 鑑定済みの接辞の数と、i 番目の接辞（未鑑定なら 0 個）
int game_item_affix_count(const Item *it);
const AffixDef *game_item_affix(const Item *it, int i, int *value);
// 同じセットの装備を何個つけているか（セット装備でなければ 0）
int game_set_pieces_equipped(const Item *it);
const SetDef *game_item_set(const Item *it);       // セット装備でなければ NULL
// 装備中の物すべてから得ている特殊効果の合計（上限つき）
int game_effect_total(Effect fx);
// 効果の表示用の文字列（"Stride +10%" など）
void game_effect_text(Effect fx, int value, char *buf, size_t size);

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

// ---- 保管庫（町でだけ出し入れできる。死んでも失わない） ----
int game_stash_count(void);
const Item *game_stash(int i);
bool game_stash_put(int bag_index);     // 持ち物 → 保管庫
bool game_stash_take(int stash_index);  // 保管庫 → 持ち物

// ---- 鍛冶屋（+1〜+10。失敗してもお金が減るだけで、壊れない） ----
typedef enum { UPGRADE_OK, UPGRADE_FAILED, UPGRADE_NO_GOLD, UPGRADE_MAX, UPGRADE_NONE } UpgradeResult;
int game_upgrade_cost(const Item *it);     // 次の段階へのお金。強化できなければ 0
int game_upgrade_chance(const Item *it);   // 成功率（%）
UpgradeResult game_upgrade_equipped(int slot);
UpgradeResult game_upgrade_bag(int bag_index);

// ---- 図鑑（一度手に入れた物だけ名前が分かる） ----
//   番号: 0〜基本アイテム数-1 が基本アイテム、その後にセット・固有装備
int game_codex_size(void);
int game_codex_seen_count(void);
bool game_codex_seen(int i);
void game_codex_name(int i, char *buf, size_t size);
int game_codex_rarity(int i);

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
