#pragma once
// ============================================================
// アイテムのカタログ（図鑑 1000種）
//   基本アイテム 900種 = 形 150 × 素材 6段階
//   固有装備 64種 + セット装備 36種（12セット × 3部位）
//   Item.base の番号: 1〜900 が基本アイテム、901〜1000 が固有・セット
//   game.h から、EquipSlot と Item の定義の後に読み込む
// ============================================================

#define TIER_COUNT 6
#define ANY_DUNGEON 0xFF

// ---- 素材の系統 ----
typedef enum { FAM_METAL, FAM_SOFT, FAM_WOOD, FAM_JEWEL, FAM_COUNT } Family;

// ---- 特殊効果 ----
typedef enum {
  FX_NONE = 0,
  FX_STRIDE,   // 1歩を多めに数える（%）
  FX_GOLD,     // 拾うお金（%）
  FX_MAGIC,    // レア度の抽選（%）
  FX_XP,       // 経験値（%）
  FX_LEECH,    // 戦闘に勝つと最大HPの % を回復
  FX_GUARD,    // 戦闘で受けるダメージ −%
  FX_TRAP,     // 罠のダメージ −%
  FX_POTION,   // ポーションの回復量 +%
  FX_ATK,      // 攻撃 +%
  FX_DEF,      // 防御 +%
  FX_HP,       // 最大HP +%
  FX_ALL,      // 攻撃・防御・最大HP +%
  FX_COUNT,
} Effect;

// ---- 形（150種） ----
//   X(ID, 名前, 装備枠, 系統, 格0〜3, ATK%, DEF%, HP%)　絵は tools/gb_items.py
//   格が高いほど出にくい。性能は装備枠ごとの基本値に % を掛ける（その枠にない性能は 0 のまま）
#define SHAPE_LIST(X) \
  /* 剣 */ \
  X(SHORT_SWORD, "Short Sword", SLOT_WEAPON, FAM_METAL, 0, 100, 100, 100) \
  X(GLADIUS, "Gladius", SLOT_WEAPON, FAM_METAL, 0, 105, 100, 100) \
  X(SABER, "Saber", SLOT_WEAPON, FAM_METAL, 1, 105, 100, 100) \
  X(SCIMITAR, "Scimitar", SLOT_WEAPON, FAM_METAL, 1, 110, 100, 100) \
  X(FALCHION, "Falchion", SLOT_WEAPON, FAM_METAL, 1, 112, 100, 100) \
  X(RAPIER, "Rapier", SLOT_WEAPON, FAM_METAL, 2, 108, 100, 100) \
  X(LONG_SWORD, "Long Sword", SLOT_WEAPON, FAM_METAL, 2, 115, 100, 100) \
  X(BROADSWORD, "Broadsword", SLOT_WEAPON, FAM_METAL, 2, 118, 100, 100) \
  X(KATANA, "Katana", SLOT_WEAPON, FAM_METAL, 3, 122, 100, 100) \
  X(CLAYMORE, "Claymore", SLOT_WEAPON, FAM_METAL, 3, 130, 100, 100) \
  X(FLAMBERGE, "Flamberge", SLOT_WEAPON, FAM_METAL, 3, 135, 100, 100) \
  /* 短剣 */ \
  X(KNIFE, "Knife", SLOT_WEAPON, FAM_METAL, 0, 85, 100, 100) \
  X(DAGGER, "Dagger", SLOT_WEAPON, FAM_METAL, 0, 90, 100, 100) \
  X(DIRK, "Dirk", SLOT_WEAPON, FAM_METAL, 1, 95, 100, 100) \
  X(STILETTO, "Stiletto", SLOT_WEAPON, FAM_METAL, 1, 100, 100, 100) \
  X(KRIS, "Kris", SLOT_WEAPON, FAM_METAL, 2, 105, 100, 100) \
  X(KUKRI, "Kukri", SLOT_WEAPON, FAM_METAL, 3, 110, 100, 100) \
  /* 斧 */ \
  X(HATCHET, "Hatchet", SLOT_WEAPON, FAM_METAL, 0, 95, 100, 100) \
  X(HAND_AXE, "Hand Axe", SLOT_WEAPON, FAM_METAL, 0, 100, 100, 100) \
  X(CLEAVER, "Cleaver", SLOT_WEAPON, FAM_METAL, 1, 108, 100, 100) \
  X(WAR_AXE, "War Axe", SLOT_WEAPON, FAM_METAL, 1, 112, 100, 100) \
  X(BEARDED_AXE, "Bearded Axe", SLOT_WEAPON, FAM_METAL, 2, 118, 100, 100) \
  X(BATTLE_AXE, "Battle Axe", SLOT_WEAPON, FAM_METAL, 2, 125, 100, 100) \
  X(GREAT_AXE, "Great Axe", SLOT_WEAPON, FAM_METAL, 3, 135, 100, 100) \
  /* 鈍器 */ \
  X(CLUB, "Club", SLOT_WEAPON, FAM_METAL, 0, 90, 100, 100) \
  X(CUDGEL, "Cudgel", SLOT_WEAPON, FAM_METAL, 0, 95, 100, 100) \
  X(MACE, "Mace", SLOT_WEAPON, FAM_METAL, 1, 105, 100, 100) \
  X(FLAIL, "Flail", SLOT_WEAPON, FAM_METAL, 1, 110, 100, 100) \
  X(MORNINGSTAR, "Morningstar", SLOT_WEAPON, FAM_METAL, 2, 118, 100, 100) \
  X(WAR_HAMMER, "War Hammer", SLOT_WEAPON, FAM_METAL, 2, 122, 100, 100) \
  X(MAUL, "Maul", SLOT_WEAPON, FAM_METAL, 3, 130, 100, 100) \
  X(GREAT_MAUL, "Great Maul", SLOT_WEAPON, FAM_METAL, 3, 140, 100, 100) \
  /* 長柄 */ \
  X(SPEAR, "Spear", SLOT_WEAPON, FAM_METAL, 0, 100, 100, 100) \
  X(PIKE, "Pike", SLOT_WEAPON, FAM_METAL, 1, 108, 100, 100) \
  X(TRIDENT, "Trident", SLOT_WEAPON, FAM_METAL, 1, 112, 100, 100) \
  X(LANCE, "Lance", SLOT_WEAPON, FAM_METAL, 2, 118, 100, 100) \
  X(HALBERD, "Halberd", SLOT_WEAPON, FAM_METAL, 2, 125, 100, 100) \
  X(GLAIVE, "Glaive", SLOT_WEAPON, FAM_METAL, 2, 122, 100, 100) \
  X(PARTISAN, "Partisan", SLOT_WEAPON, FAM_METAL, 3, 128, 100, 100) \
  X(SCYTHE, "Scythe", SLOT_WEAPON, FAM_METAL, 3, 135, 100, 100) \
  /* 杖 */ \
  X(STAFF, "Staff", SLOT_WEAPON, FAM_WOOD, 0, 90, 100, 100) \
  X(ROD, "Rod", SLOT_WEAPON, FAM_WOOD, 0, 95, 100, 100) \
  X(WAND, "Wand", SLOT_WEAPON, FAM_WOOD, 1, 100, 100, 100) \
  X(CROOK, "Crook", SLOT_WEAPON, FAM_WOOD, 2, 108, 100, 100) \
  X(GREAT_STAFF, "Great Staff", SLOT_WEAPON, FAM_WOOD, 3, 120, 100, 100) \
  /* 拳 */ \
  X(KNUCKLES, "Knuckles", SLOT_WEAPON, FAM_METAL, 0, 90, 100, 100) \
  X(CESTUS, "Cestus", SLOT_WEAPON, FAM_METAL, 1, 98, 100, 100) \
  X(CLAWS, "Claws", SLOT_WEAPON, FAM_METAL, 1, 105, 100, 100) \
  X(KATAR, "Katar", SLOT_WEAPON, FAM_METAL, 2, 115, 100, 100) \
  X(TALONS, "Talons", SLOT_WEAPON, FAM_METAL, 3, 125, 100, 100) \
  /* 盾 */ \
  X(BUCKLER, "Buckler", SLOT_OFFHAND, FAM_METAL, 0, 100, 80, 120) \
  X(TARGE, "Targe", SLOT_OFFHAND, FAM_METAL, 0, 100, 90, 110) \
  X(ROUND_SHIELD, "Round Shield", SLOT_OFFHAND, FAM_METAL, 1, 100, 100, 100) \
  X(HEATER, "Heater", SLOT_OFFHAND, FAM_METAL, 1, 100, 108, 100) \
  X(KITE_SHIELD, "Kite Shield", SLOT_OFFHAND, FAM_METAL, 2, 100, 115, 105) \
  X(SPIKED_SHIELD, "Spiked Shield", SLOT_OFFHAND, FAM_METAL, 2, 100, 112, 95) \
  X(SCUTUM, "Scutum", SLOT_OFFHAND, FAM_METAL, 2, 100, 120, 110) \
  X(PAVISE, "Pavise", SLOT_OFFHAND, FAM_METAL, 3, 100, 130, 110) \
  X(TOWER_SHIELD, "Tower Shield", SLOT_OFFHAND, FAM_METAL, 3, 100, 140, 100) \
  X(BULWARK, "Bulwark", SLOT_OFFHAND, FAM_METAL, 3, 100, 135, 125) \
  /* 触媒 */ \
  X(ORB, "Orb", SLOT_OFFHAND, FAM_JEWEL, 1, 100, 60, 150) \
  X(TOME, "Tome", SLOT_OFFHAND, FAM_JEWEL, 1, 100, 50, 160) \
  X(IDOL, "Idol", SLOT_OFFHAND, FAM_JEWEL, 2, 100, 70, 150) \
  X(LANTERN, "Lantern", SLOT_OFFHAND, FAM_JEWEL, 3, 100, 80, 150) \
  /* 頭 */ \
  X(CAP, "Cap", SLOT_HEAD, FAM_SOFT, 0, 100, 90, 100) \
  X(HOOD, "Hood", SLOT_HEAD, FAM_SOFT, 0, 100, 95, 100) \
  X(BANDANA, "Bandana", SLOT_HEAD, FAM_SOFT, 1, 100, 90, 100) \
  X(MASK, "Mask", SLOT_HEAD, FAM_SOFT, 1, 100, 100, 100) \
  X(WIZARD_HAT, "Wizard Hat", SLOT_HEAD, FAM_SOFT, 2, 100, 105, 100) \
  X(CIRCLET, "Circlet", SLOT_HEAD, FAM_JEWEL, 2, 100, 100, 100) \
  X(CROWN, "Crown", SLOT_HEAD, FAM_JEWEL, 3, 100, 120, 100) \
  X(COIF, "Coif", SLOT_HEAD, FAM_METAL, 0, 100, 105, 100) \
  X(SKULLCAP, "Skullcap", SLOT_HEAD, FAM_METAL, 0, 100, 110, 100) \
  X(SALLET, "Sallet", SLOT_HEAD, FAM_METAL, 1, 100, 115, 100) \
  X(BARBUTE, "Barbute", SLOT_HEAD, FAM_METAL, 1, 100, 120, 100) \
  X(HORNED_HELM, "Horned Helm", SLOT_HEAD, FAM_METAL, 2, 100, 125, 100) \
  X(WINGED_HELM, "Winged Helm", SLOT_HEAD, FAM_METAL, 2, 100, 128, 100) \
  X(GREAT_HELM, "Great Helm", SLOT_HEAD, FAM_METAL, 3, 100, 140, 100) \
  /* 胴 */ \
  X(TUNIC, "Tunic", SLOT_BODY, FAM_SOFT, 0, 100, 70, 110) \
  X(ROBE, "Robe", SLOT_BODY, FAM_SOFT, 0, 100, 60, 130) \
  X(CLOAK, "Cloak", SLOT_BODY, FAM_SOFT, 1, 100, 75, 120) \
  X(MANTLE, "Mantle", SLOT_BODY, FAM_SOFT, 1, 100, 80, 125) \
  X(VEST, "Vest", SLOT_BODY, FAM_SOFT, 1, 100, 85, 110) \
  X(JERKIN, "Jerkin", SLOT_BODY, FAM_SOFT, 2, 100, 95, 110) \
  X(GAMBESON, "Gambeson", SLOT_BODY, FAM_SOFT, 2, 100, 100, 115) \
  X(BRIGANDINE, "Brigandine", SLOT_BODY, FAM_METAL, 0, 100, 105, 100) \
  X(RING_MAIL, "Ring Mail", SLOT_BODY, FAM_METAL, 0, 100, 110, 95) \
  X(CHAIN_MAIL, "Chain Mail", SLOT_BODY, FAM_METAL, 1, 100, 118, 95) \
  X(SCALE_MAIL, "Scale Mail", SLOT_BODY, FAM_METAL, 1, 100, 122, 100) \
  X(LAMELLAR, "Lamellar", SLOT_BODY, FAM_METAL, 2, 100, 125, 100) \
  X(CUIRASS, "Cuirass", SLOT_BODY, FAM_METAL, 2, 100, 130, 95) \
  X(BREASTPLATE, "Breastplate", SLOT_BODY, FAM_METAL, 2, 100, 135, 95) \
  X(HALF_PLATE, "Half Plate", SLOT_BODY, FAM_METAL, 3, 100, 140, 100) \
  X(FULL_PLATE, "Full Plate", SLOT_BODY, FAM_METAL, 3, 100, 150, 100) \
  /* 手 */ \
  X(WRAPS, "Wraps", SLOT_HANDS, FAM_SOFT, 0, 110, 70, 100) \
  X(GLOVES, "Gloves", SLOT_HANDS, FAM_SOFT, 0, 100, 90, 100) \
  X(MITTENS, "Mittens", SLOT_HANDS, FAM_SOFT, 0, 80, 110, 100) \
  X(GRIPS, "Grips", SLOT_HANDS, FAM_SOFT, 1, 115, 85, 100) \
  X(BRACERS, "Bracers", SLOT_HANDS, FAM_SOFT, 1, 100, 105, 100) \
  X(ARMGUARDS, "Armguards", SLOT_HANDS, FAM_SOFT, 2, 95, 120, 100) \
  X(VAMBRACES, "Vambraces", SLOT_HANDS, FAM_METAL, 1, 100, 125, 100) \
  X(CHAIN_GLOVES, "Chain Gloves", SLOT_HANDS, FAM_METAL, 1, 105, 115, 100) \
  X(GAUNTLETS, "Gauntlets", SLOT_HANDS, FAM_METAL, 2, 110, 130, 100) \
  X(HANDGUARDS, "Handguards", SLOT_HANDS, FAM_METAL, 2, 95, 140, 100) \
  X(WARFISTS, "Warfists", SLOT_HANDS, FAM_METAL, 3, 135, 110, 100) \
  X(CRUSHERS, "Crushers", SLOT_HANDS, FAM_METAL, 3, 145, 100, 100) \
  /* 足 */ \
  X(SANDALS, "Sandals", SLOT_FEET, FAM_SOFT, 0, 100, 70, 100) \
  X(SHOES, "Shoes", SLOT_FEET, FAM_SOFT, 0, 100, 85, 100) \
  X(SLIPPERS, "Slippers", SLOT_FEET, FAM_SOFT, 0, 100, 75, 100) \
  X(BOOTS, "Boots", SLOT_FEET, FAM_SOFT, 1, 100, 100, 100) \
  X(TREADS, "Treads", SLOT_FEET, FAM_SOFT, 2, 100, 105, 100) \
  X(STRIDERS, "Striders", SLOT_FEET, FAM_SOFT, 2, 100, 110, 100) \
  X(GREAVES, "Greaves", SLOT_FEET, FAM_METAL, 1, 100, 115, 100) \
  X(SABATONS, "Sabatons", SLOT_FEET, FAM_METAL, 2, 100, 125, 100) \
  X(CHAIN_BOOTS, "Chain Boots", SLOT_FEET, FAM_METAL, 1, 100, 118, 100) \
  X(WAR_BOOTS, "War Boots", SLOT_FEET, FAM_METAL, 2, 100, 128, 100) \
  X(STOMPERS, "Stompers", SLOT_FEET, FAM_METAL, 3, 100, 135, 100) \
  X(SPURRED_BOOTS, "Spurred Boots", SLOT_FEET, FAM_METAL, 3, 100, 140, 100) \
  /* 首飾り */ \
  X(CHARM, "Charm", SLOT_AMULET, FAM_JEWEL, 0, 100, 100, 90) \
  X(PENDANT, "Pendant", SLOT_AMULET, FAM_JEWEL, 0, 100, 100, 95) \
  X(LOCKET, "Locket", SLOT_AMULET, FAM_JEWEL, 0, 100, 100, 100) \
  X(CHOKER, "Choker", SLOT_AMULET, FAM_JEWEL, 1, 100, 100, 100) \
  X(TORC, "Torc", SLOT_AMULET, FAM_JEWEL, 1, 100, 100, 105) \
  X(AMULET, "Amulet", SLOT_AMULET, FAM_JEWEL, 1, 100, 100, 108) \
  X(MEDALLION, "Medallion", SLOT_AMULET, FAM_JEWEL, 1, 100, 100, 110) \
  X(TALISMAN, "Talisman", SLOT_AMULET, FAM_JEWEL, 2, 100, 100, 112) \
  X(SCARAB, "Scarab", SLOT_AMULET, FAM_JEWEL, 2, 100, 100, 115) \
  X(ROSARY, "Rosary", SLOT_AMULET, FAM_JEWEL, 2, 100, 100, 115) \
  X(FANG_CHAIN, "Fang Chain", SLOT_AMULET, FAM_JEWEL, 2, 100, 100, 118) \
  X(TOTEM_CORD, "Totem Cord", SLOT_AMULET, FAM_JEWEL, 2, 100, 100, 118) \
  X(EYE_PENDANT, "Eye Pendant", SLOT_AMULET, FAM_JEWEL, 3, 100, 100, 122) \
  X(SUN_DISC, "Sun Disc", SLOT_AMULET, FAM_JEWEL, 3, 100, 100, 125) \
  X(MOON_DISC, "Moon Disc", SLOT_AMULET, FAM_JEWEL, 3, 100, 100, 125) \
  X(HEART_STONE, "Heart Stone", SLOT_AMULET, FAM_JEWEL, 3, 100, 100, 130) \
  /* 指輪 */ \
  X(BAND, "Band", SLOT_RING1, FAM_JEWEL, 0, 90, 100, 100) \
  X(RING, "Ring", SLOT_RING1, FAM_JEWEL, 0, 95, 100, 100) \
  X(LOOP, "Loop", SLOT_RING1, FAM_JEWEL, 0, 100, 100, 100) \
  X(COIL, "Coil", SLOT_RING1, FAM_JEWEL, 1, 100, 100, 100) \
  X(TWIST, "Twist", SLOT_RING1, FAM_JEWEL, 1, 105, 100, 100) \
  X(SPIRAL, "Spiral", SLOT_RING1, FAM_JEWEL, 1, 105, 100, 100) \
  X(KNOT, "Knot", SLOT_RING1, FAM_JEWEL, 1, 108, 100, 100) \
  X(SIGNET, "Signet", SLOT_RING1, FAM_JEWEL, 2, 112, 100, 100) \
  X(SEAL, "Seal", SLOT_RING1, FAM_JEWEL, 2, 112, 100, 100) \
  X(GEM_RING, "Gem Ring", SLOT_RING1, FAM_JEWEL, 2, 115, 100, 100) \
  X(CLAW_RING, "Claw Ring", SLOT_RING1, FAM_JEWEL, 2, 120, 100, 100) \
  X(EYE_RING, "Eye Ring", SLOT_RING1, FAM_JEWEL, 3, 118, 100, 100) \
  X(SKULL_RING, "Skull Ring", SLOT_RING1, FAM_JEWEL, 3, 122, 100, 100) \
  X(STAR_RING, "Star Ring", SLOT_RING1, FAM_JEWEL, 3, 125, 100, 100) \
  X(MOON_RING, "Moon Ring", SLOT_RING1, FAM_JEWEL, 3, 125, 100, 100) \
  X(SUN_RING, "Sun Ring", SLOT_RING1, FAM_JEWEL, 3, 130, 100, 100)

typedef enum {
#define X(id, name, slot, fam, rank, atk, def, hp) SH_##id,
  SHAPE_LIST(X)
#undef X
  SHAPE_COUNT
} ShapeId;

#define BASE_COUNT (SHAPE_COUNT * TIER_COUNT)   // 900
#define SPECIAL_BASE (BASE_COUNT + 1)           // 固有・セットの base 番号は 901 から

// 名前は表に持たない（リソース item_names.bin から item_shape_name などで読む）
typedef struct {
  uint8_t slot;      // EquipSlot（指輪は SLOT_RING1）
  uint8_t family;    // Family
  uint8_t rank;      // 0〜3。高いほど出にくい
  uint8_t atk_pct, def_pct, hp_pct;
} ShapeDef;

// ---- 固有装備・セット装備 ----
typedef struct {
  uint8_t shape;       // ShapeId
  uint8_t set_id;      // 0 = 固有装備、1〜 = g_sets の番号 + 1
  uint8_t dungeon;     // よく出るダンジョン（ANY_DUNGEON = どこでも）
  uint8_t boss;        // 1 = そのダンジョンのボスからだけ出る
  uint8_t bonus_pct;   // 基本性能の倍率（%）
  uint8_t fx1, val1;   // 特殊効果（Effect と値）
  uint8_t fx2, val2;
} SpecialDef;

typedef struct {
  uint8_t fx2, val2;   // 2部位そろえたとき
  uint8_t fx3, val3;   // 3部位そろえたとき（2部位の分に加えて）
} SetDef;

extern const ShapeDef g_shapes[SHAPE_COUNT];
extern const SpecialDef g_specials[];
extern const int g_special_count;
extern const SetDef g_sets[];
extern const int g_set_count;
extern const char *const g_effect_names[FX_COUNT];

// アイテムLv → 素材の段階（0〜5）
int item_tier_for_level(int ilvl);

// ---- 名前 ----
//   アプリ本体の RAM を節約するため、名前はリソース（item_names.bin）に置いて使うときに読む。
//   正本はこのファイルの SHAPE_LIST と items.c の SPECIAL / SET_DEF。tools/gen_art.py が書き出す
#define ITEM_NAME_LEN 21   // 20文字 + 終端。buf はこの大きさ以上
void item_shape_name(int shape, char *buf);
void item_special_name(int special, char *buf);
void item_set_name(int set_index, char *buf);          // g_sets の添字
void item_material_name(int family, int tier, char *buf);
