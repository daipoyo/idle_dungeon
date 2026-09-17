#include "game.h"
#include "item_art.h"

// ============================================================
// アイテムのカタログ（items.h の説明を参照）
// ============================================================

const ShapeDef g_shapes[SHAPE_COUNT] = {
#define X(id, name, slot, fam, rank, atk, def, hp) { slot, fam, rank, atk, def, hp },
  SHAPE_LIST(X)
#undef X
};

int item_tier_for_level(int ilvl) {
  if (ilvl <= 8) return 0;
  if (ilvl <= 16) return 1;
  if (ilvl <= 25) return 2;
  if (ilvl <= 34) return 3;
  if (ilvl <= 42) return 4;
  return 5;
}

const char *const g_effect_names[FX_COUNT] = {
  "", "Stride", "Gold", "Magic", "XP", "Leech", "Guard", "Trap", "Potion", "ATK", "DEF", "HP", "All",
};

// 性能の倍率: 普通 150、「大」185、「特大」225、セット部位 130
#define U_NORMAL 150
#define U_BIG 185
#define U_HUGE 225
#define SET_PART 130

// ダンジョンの番号（g_dungeons の順）
enum { D_CELLAR, D_WARREN, D_CRYPT, D_FUNGAL, D_DROWNED, D_FORGE, D_FROST, D_ABYSS };

// 名前はソースに残すが、表には入らない（tools/gen_art.py がリソースへ書き出す）
#define SPECIAL(name, shape, set, dungeon, boss, pct, fx1, v1, fx2, v2) \
  { shape, set, dungeon, boss, pct, fx1, v1, fx2, v2 }
#define SET_DEF(name, fx2, v2, fx3, v3) { fx2, v2, fx3, v3 }

const SpecialDef g_specials[] = {
  // ---- 固有装備: Mossy Cellar ----
  SPECIAL("Rat King's Crown", SH_CROWN, 0, D_CELLAR, 1, U_NORMAL, FX_XP, 10, 0, 0),
  SPECIAL("Ratcatcher", SH_KNIFE, 0, D_CELLAR, 0, U_NORMAL, FX_GOLD, 20, 0, 0),
  SPECIAL("Slimeskin Shoes", SH_SHOES, 0, D_CELLAR, 0, U_NORMAL, FX_TRAP, 50, 0, 0),
  SPECIAL("Batwing Cloak", SH_CLOAK, 0, D_CELLAR, 0, U_NORMAL, FX_STRIDE, 5, 0, 0),
  SPECIAL("Cellar Lantern", SH_LANTERN, 0, D_CELLAR, 0, U_NORMAL, FX_MAGIC, 10, 0, 0),
  SPECIAL("Mossbitten Club", SH_CLUB, 0, D_CELLAR, 0, U_NORMAL, FX_LEECH, 2, 0, 0),
  SPECIAL("Last Candle", SH_CHARM, 0, D_CELLAR, 0, U_NORMAL, FX_POTION, 20, 0, 0),
  SPECIAL("Pot Lid", SH_BUCKLER, 0, D_CELLAR, 0, U_NORMAL, FX_GUARD, 5, 0, 0),
  // ---- Goblin Warren ----
  SPECIAL("Chief's Cleaver", SH_CLEAVER, 0, D_WARREN, 1, U_BIG, FX_LEECH, 2, 0, 0),
  SPECIAL("Stolen Signet", SH_SIGNET, 0, D_WARREN, 0, U_NORMAL, FX_GOLD, 30, 0, 0),
  SPECIAL("Hobnail Stompers", SH_STOMPERS, 0, D_WARREN, 0, U_NORMAL, FX_GUARD, 6, 0, 0),
  SPECIAL("Sneakgrip", SH_GRIPS, 0, D_WARREN, 0, U_NORMAL, FX_MAGIC, 15, 0, 0),
  SPECIAL("Fletcher's Hood", SH_HOOD, 0, D_WARREN, 0, U_NORMAL, FX_XP, 10, 0, 0),
  SPECIAL("Loot Mittens", SH_MITTENS, 0, D_WARREN, 0, U_NORMAL, FX_GOLD, 20, 0, 0),
  SPECIAL("Tunnel Torc", SH_TORC, 0, D_WARREN, 0, U_NORMAL, FX_STRIDE, 8, 0, 0),
  SPECIAL("Warren Targe", SH_TARGE, 0, D_WARREN, 0, U_NORMAL, FX_TRAP, 50, 0, 0),
  // ---- Sunken Crypt ----
  SPECIAL("Bone Lord's Rod", SH_ROD, 0, D_CRYPT, 1, U_NORMAL, FX_XP, 15, 0, 0),
  SPECIAL("Gravebind", SH_CHAIN_MAIL, 0, D_CRYPT, 0, 170, FX_HP, 8, 0, 0),
  SPECIAL("Wraithveil", SH_MASK, 0, D_CRYPT, 0, U_NORMAL, FX_GUARD, 8, 0, 0),
  SPECIAL("Ghoulclaw", SH_CLAWS, 0, D_CRYPT, 0, U_NORMAL, FX_LEECH, 4, 0, 0),
  SPECIAL("Coffin Nail", SH_STILETTO, 0, D_CRYPT, 0, U_BIG, 0, 0, 0, 0),
  SPECIAL("Mourning Locket", SH_LOCKET, 0, D_CRYPT, 0, U_NORMAL, FX_POTION, 30, 0, 0),
  SPECIAL("Dirge Coif", SH_COIF, 0, D_CRYPT, 0, 170, FX_HP, 8, FX_TRAP, 30),
  SPECIAL("Ossuary Ring", SH_SKULL_RING, 0, D_CRYPT, 0, U_NORMAL, FX_MAGIC, 20, 0, 0),
  // ---- Fungal Caverns ----
  SPECIAL("Spore Heart", SH_HEART_STONE, 0, D_FUNGAL, 1, U_NORMAL, FX_LEECH, 5, 0, 0),
  SPECIAL("Mycelium Wraps", SH_WRAPS, 0, D_FUNGAL, 0, U_NORMAL, FX_STRIDE, 10, 0, 0),
  SPECIAL("Puffball Pavise", SH_PAVISE, 0, D_FUNGAL, 0, U_NORMAL, FX_GUARD, 10, 0, 0),
  SPECIAL("Rotbloom Staff", SH_STAFF, 0, D_FUNGAL, 0, U_NORMAL, FX_XP, 15, 0, 0),
  SPECIAL("Glowcap Circlet", SH_CIRCLET, 0, D_FUNGAL, 0, U_NORMAL, FX_MAGIC, 20, 0, 0),
  SPECIAL("Moldhide Jerkin", SH_JERKIN, 0, D_FUNGAL, 0, U_NORMAL, FX_TRAP, 60, 0, 0),
  SPECIAL("Sporeburst Flail", SH_FLAIL, 0, D_FUNGAL, 0, U_BIG, 0, 0, 0, 0),
  SPECIAL("Toadstool Loop", SH_LOOP, 0, D_FUNGAL, 0, U_NORMAL, FX_POTION, 40, 0, 0),
  // ---- Drowned Temple ----
  SPECIAL("Tidecaller", SH_TRIDENT, 0, D_DROWNED, 1, U_BIG, FX_LEECH, 3, 0, 0),
  SPECIAL("Tide Pearl", SH_PENDANT, 0, D_DROWNED, 0, U_NORMAL, FX_POTION, 50, 0, 0),
  SPECIAL("Barnacle Bulwark", SH_BULWARK, 0, D_DROWNED, 0, U_NORMAL, FX_GUARD, 12, 0, 0),
  SPECIAL("Eelskin Treads", SH_TREADS, 0, D_DROWNED, 0, U_NORMAL, FX_STRIDE, 12, 0, 0),
  SPECIAL("Coral Barbute", SH_BARBUTE, 0, D_DROWNED, 0, U_BIG, 0, 0, 0, 0),
  SPECIAL("Siren's Coil", SH_COIL, 0, D_DROWNED, 0, U_NORMAL, FX_MAGIC, 25, 0, 0),
  SPECIAL("Leviathan Scale", SH_SCALE_MAIL, 0, D_DROWNED, 0, 170, FX_HP, 10, 0, 0),
  SPECIAL("Undertow", SH_GLAIVE, 0, D_DROWNED, 0, U_NORMAL, FX_GOLD, 40, 0, 0),
  // ---- Ember Forge ----
  SPECIAL("Colossus Maul", SH_GREAT_MAUL, 0, D_FORGE, 1, U_HUGE, 0, 0, 0, 0),
  SPECIAL("Anvilbreaker", SH_WAR_HAMMER, 0, D_FORGE, 0, U_BIG, FX_GUARD, 5, 0, 0),
  SPECIAL("Forgemaster Vest", SH_VEST, 0, D_FORGE, 0, U_NORMAL, FX_GUARD, 14, 0, 0),
  SPECIAL("Cinder Sabatons", SH_SABATONS, 0, D_FORGE, 0, U_NORMAL, FX_TRAP, 80, 0, 0),
  SPECIAL("Imp Tongue", SH_KRIS, 0, D_FORGE, 0, U_NORMAL, FX_LEECH, 5, 0, 0),
  SPECIAL("Slag Medallion", SH_MEDALLION, 0, D_FORGE, 0, U_NORMAL, FX_XP, 20, 0, 0),
  SPECIAL("Molten Knuckles", SH_KNUCKLES, 0, D_FORGE, 0, U_BIG, 0, 0, 0, 0),
  SPECIAL("Golem Seal", SH_SEAL, 0, D_FORGE, 0, U_NORMAL, FX_HP, 12, 0, 0),
  // ---- Frost Spire ----
  SPECIAL("Wyrmfrost", SH_CLAYMORE, 0, D_FROST, 1, U_HUGE, 0, 0, 0, 0),
  SPECIAL("Wolfpelt Mantle", SH_MANTLE, 0, D_FROST, 0, U_NORMAL, FX_STRIDE, 15, 0, 0),
  SPECIAL("Trollblood Cuirass", SH_CUIRASS, 0, D_FROST, 0, U_NORMAL, FX_LEECH, 6, 0, 0),
  SPECIAL("Yeti Mittens", SH_MITTENS, 0, D_FROST, 0, U_NORMAL, FX_GUARD, 15, 0, 0),
  SPECIAL("Icicle", SH_RAPIER, 0, D_FROST, 0, U_BIG, 0, 0, 0, 0),
  SPECIAL("Winter's Eye", SH_EYE_PENDANT, 0, D_FROST, 0, U_NORMAL, FX_MAGIC, 35, 0, 0),
  SPECIAL("Avalanche", SH_TOWER_SHIELD, 0, D_FROST, 0, U_HUGE, 0, 0, 0, 0),
  SPECIAL("Frostbite Band", SH_BAND, 0, D_FROST, 0, U_NORMAL, FX_XP, 25, 0, 0),
  // ---- Abyssal Gate ----
  SPECIAL("Doomcrown", SH_CROWN, 0, D_ABYSS, 1, U_BIG, FX_ALL, 10, FX_XP, 20),
  SPECIAL("Hellhound Collar", SH_CHOKER, 0, D_ABYSS, 0, U_NORMAL, FX_LEECH, 8, 0, 0),
  SPECIAL("Fiendfang", SH_KATAR, 0, D_ABYSS, 0, U_HUGE, 0, 0, 0, 0),
  SPECIAL("Soulreaver", SH_SCYTHE, 0, D_ABYSS, 0, U_NORMAL, FX_LEECH, 5, FX_XP, 15),
  SPECIAL("Gatewarden", SH_FULL_PLATE, 0, D_ABYSS, 0, U_NORMAL, FX_GUARD, 20, 0, 0),
  SPECIAL("Brimstone Ring", SH_SUN_RING, 0, D_ABYSS, 0, U_NORMAL, FX_GOLD, 60, 0, 0),
  SPECIAL("Ashwalk Greaves", SH_GREAVES, 0, D_ABYSS, 0, U_NORMAL, FX_STRIDE, 20, 0, 0),
  SPECIAL("Last Vigil", SH_GREAT_STAFF, 0, D_ABYSS, 0, U_NORMAL, FX_MAGIC, 50, 0, 0),

  // ---- セット装備 ----
  SPECIAL("Scout Hood", SH_HOOD, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Scout Knife", SH_KNIFE, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Scout Shoes", SH_SHOES, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Raider Cap", SH_CAP, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Raider Hatchet", SH_HATCHET, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Raider Targe", SH_TARGE, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Grave Coif", SH_COIF, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Grave Mail", SH_RING_MAIL, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Grave Band", SH_BAND, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Spore Mask", SH_MASK, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Spore Robe", SH_ROBE, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Spore Wand", SH_WAND, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Tide Sallet", SH_SALLET, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Tide Scale", SH_SCALE_MAIL, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Tide Pike", SH_PIKE, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Anvil Helm", SH_GREAT_HELM, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Anvil Plate", SH_BREASTPLATE, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Anvil Fists", SH_GAUNTLETS, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Slayer Spear", SH_SPEAR, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Slayer Bracers", SH_VAMBRACES, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Slayer Striders", SH_STRIDERS, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Court Circlet", SH_CIRCLET, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Court Mantle", SH_MANTLE, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Court Signet", SH_SIGNET, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Hellforge Maul", SH_MAUL, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Hellforge Helm", SH_HORNED_HELM, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Hellforge Plate", SH_FULL_PLATE, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Pilgrim Staff", SH_STAFF, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Pilgrim Cloak", SH_CLOAK, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Pilgrim Sandals", SH_SANDALS, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Seeker Gloves", SH_GLOVES, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Seeker Locket", SH_LOCKET, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Seeker Loop", SH_LOOP, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Sentinel Bulwark", SH_BULWARK, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Sentinel Greaves", SH_GREAVES, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
  SPECIAL("Sentinel Torc", SH_TORC, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0),
};
const int g_special_count = sizeof(g_specials) / sizeof(g_specials[0]);

// 3部位の効果は、2部位の効果に加えてつく（Pilgrim は合計で Stride +25%）
const SetDef g_sets[] = {
  SET_DEF("Cellar Scout", FX_STRIDE, 5, FX_GOLD, 20),
  SET_DEF("Goblin Raider", FX_GOLD, 15, FX_MAGIC, 15),
  SET_DEF("Gravewalker", FX_HP, 10, FX_LEECH, 4),
  SET_DEF("Mycologist", FX_POTION, 30, FX_XP, 15),
  SET_DEF("Tidebound", FX_GUARD, 8, FX_STRIDE, 10),
  SET_DEF("Anvilborn", FX_DEF, 15, FX_GUARD, 15),
  SET_DEF("Wyrmslayer", FX_ATK, 15, FX_MAGIC, 30),
  SET_DEF("Winter Court", FX_XP, 20, FX_GOLD, 40),
  SET_DEF("Hellforge", FX_ATK, 20, FX_LEECH, 8),
  SET_DEF("Pilgrim", FX_STRIDE, 10, FX_STRIDE, 15),
  SET_DEF("Seeker", FX_GOLD, 25, FX_MAGIC, 30),
  SET_DEF("Sentinel", FX_GUARD, 10, FX_HP, 20),
};
const int g_set_count = sizeof(g_sets) / sizeof(g_sets[0]);

// ============================================================
// 名前（リソースから読む）
// ============================================================
_Static_assert(ITEM_NAME_RECORD == ITEM_NAME_LEN, "item_names.bin record size");

static void load_name(int index, char *buf) {
  static ResHandle s_names;
  if (!s_names) s_names = resource_get_handle(RESOURCE_ID_ITEM_NAMES);
  if (index < 0 || resource_load_byte_range(s_names, index * ITEM_NAME_LEN, (uint8_t *)buf,
                                            ITEM_NAME_LEN) != ITEM_NAME_LEN) {
    buf[0] = '\0';
  }
  buf[ITEM_NAME_LEN - 1] = '\0';
}

void item_shape_name(int shape, char *buf) {
  load_name(shape >= 0 && shape < SHAPE_COUNT ? ITEM_NAME_SHAPE_FIRST + shape : -1, buf);
}

void item_special_name(int special, char *buf) {
  load_name(special >= 0 && special < g_special_count ? ITEM_NAME_SPECIAL_FIRST + special : -1, buf);
}

void item_set_name(int set_index, char *buf) {
  load_name(set_index >= 0 && set_index < g_set_count ? ITEM_NAME_SET_FIRST + set_index : -1, buf);
}

void item_material_name(int family, int tier, char *buf) {
  bool ok = family >= 0 && family < FAM_COUNT && tier >= 0 && tier < TIER_COUNT;
  load_name(ok ? ITEM_NAME_MATERIAL_FIRST + family * TIER_COUNT + tier : -1, buf);
}
