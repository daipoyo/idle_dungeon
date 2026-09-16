#include "game.h"

// ============================================================
// アイテムのカタログ（items.h の説明を参照）
// ============================================================

const ShapeDef g_shapes[SHAPE_COUNT] = {
#define X(id, name, slot, fam, rank, atk, def, hp) { name, slot, fam, rank, atk, def, hp },
  SHAPE_LIST(X)
#undef X
};

// 前半3段階は系統ごと、後半3段階はダンジョンで生まれた素材として全系統で共通
const char *const g_material_names[FAM_COUNT][TIER_COUNT] = {
  [FAM_METAL] = { "Bronze", "Iron", "Steel", "Ember", "Rime", "Void" },
  [FAM_SOFT] = { "Linen", "Leather", "Silk", "Ember", "Rime", "Void" },
  [FAM_WOOD] = { "Ash", "Oak", "Yew", "Ember", "Rime", "Void" },
  [FAM_JEWEL] = { "Bone", "Copper", "Silver", "Ember", "Rime", "Void" },
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

const SpecialDef g_specials[] = {
  // ---- 固有装備: Mossy Cellar ----
  { "Rat King's Crown", SH_CROWN, 0, D_CELLAR, 1, U_NORMAL, FX_XP, 10, 0, 0 },
  { "Ratcatcher", SH_KNIFE, 0, D_CELLAR, 0, U_NORMAL, FX_GOLD, 20, 0, 0 },
  { "Slimeskin Shoes", SH_SHOES, 0, D_CELLAR, 0, U_NORMAL, FX_TRAP, 50, 0, 0 },
  { "Batwing Cloak", SH_CLOAK, 0, D_CELLAR, 0, U_NORMAL, FX_STRIDE, 5, 0, 0 },
  { "Cellar Lantern", SH_LANTERN, 0, D_CELLAR, 0, U_NORMAL, FX_MAGIC, 10, 0, 0 },
  { "Mossbitten Club", SH_CLUB, 0, D_CELLAR, 0, U_NORMAL, FX_LEECH, 2, 0, 0 },
  { "Last Candle", SH_CHARM, 0, D_CELLAR, 0, U_NORMAL, FX_POTION, 20, 0, 0 },
  { "Pot Lid", SH_BUCKLER, 0, D_CELLAR, 0, U_NORMAL, FX_GUARD, 5, 0, 0 },
  // ---- Goblin Warren ----
  { "Chief's Cleaver", SH_CLEAVER, 0, D_WARREN, 1, U_BIG, FX_LEECH, 2, 0, 0 },
  { "Stolen Signet", SH_SIGNET, 0, D_WARREN, 0, U_NORMAL, FX_GOLD, 30, 0, 0 },
  { "Hobnail Stompers", SH_STOMPERS, 0, D_WARREN, 0, U_NORMAL, FX_GUARD, 6, 0, 0 },
  { "Sneakgrip", SH_GRIPS, 0, D_WARREN, 0, U_NORMAL, FX_MAGIC, 15, 0, 0 },
  { "Fletcher's Hood", SH_HOOD, 0, D_WARREN, 0, U_NORMAL, FX_XP, 10, 0, 0 },
  { "Loot Mittens", SH_MITTENS, 0, D_WARREN, 0, U_NORMAL, FX_GOLD, 20, 0, 0 },
  { "Tunnel Torc", SH_TORC, 0, D_WARREN, 0, U_NORMAL, FX_STRIDE, 8, 0, 0 },
  { "Warren Targe", SH_TARGE, 0, D_WARREN, 0, U_NORMAL, FX_TRAP, 50, 0, 0 },
  // ---- Sunken Crypt ----
  { "Bone Lord's Rod", SH_ROD, 0, D_CRYPT, 1, U_NORMAL, FX_XP, 15, 0, 0 },
  { "Gravebind", SH_CHAIN_MAIL, 0, D_CRYPT, 0, 170, FX_HP, 8, 0, 0 },
  { "Wraithveil", SH_MASK, 0, D_CRYPT, 0, U_NORMAL, FX_GUARD, 8, 0, 0 },
  { "Ghoulclaw", SH_CLAWS, 0, D_CRYPT, 0, U_NORMAL, FX_LEECH, 4, 0, 0 },
  { "Coffin Nail", SH_STILETTO, 0, D_CRYPT, 0, U_BIG, 0, 0, 0, 0 },
  { "Mourning Locket", SH_LOCKET, 0, D_CRYPT, 0, U_NORMAL, FX_POTION, 30, 0, 0 },
  { "Dirge Coif", SH_COIF, 0, D_CRYPT, 0, 170, FX_HP, 8, FX_TRAP, 30 },
  { "Ossuary Ring", SH_SKULL_RING, 0, D_CRYPT, 0, U_NORMAL, FX_MAGIC, 20, 0, 0 },
  // ---- Fungal Caverns ----
  { "Spore Heart", SH_HEART_STONE, 0, D_FUNGAL, 1, U_NORMAL, FX_LEECH, 5, 0, 0 },
  { "Mycelium Wraps", SH_WRAPS, 0, D_FUNGAL, 0, U_NORMAL, FX_STRIDE, 10, 0, 0 },
  { "Puffball Pavise", SH_PAVISE, 0, D_FUNGAL, 0, U_NORMAL, FX_GUARD, 10, 0, 0 },
  { "Rotbloom Staff", SH_STAFF, 0, D_FUNGAL, 0, U_NORMAL, FX_XP, 15, 0, 0 },
  { "Glowcap Circlet", SH_CIRCLET, 0, D_FUNGAL, 0, U_NORMAL, FX_MAGIC, 20, 0, 0 },
  { "Moldhide Jerkin", SH_JERKIN, 0, D_FUNGAL, 0, U_NORMAL, FX_TRAP, 60, 0, 0 },
  { "Sporeburst Flail", SH_FLAIL, 0, D_FUNGAL, 0, U_BIG, 0, 0, 0, 0 },
  { "Toadstool Loop", SH_LOOP, 0, D_FUNGAL, 0, U_NORMAL, FX_POTION, 40, 0, 0 },
  // ---- Drowned Temple ----
  { "Tidecaller", SH_TRIDENT, 0, D_DROWNED, 1, U_BIG, FX_LEECH, 3, 0, 0 },
  { "Tide Pearl", SH_PENDANT, 0, D_DROWNED, 0, U_NORMAL, FX_POTION, 50, 0, 0 },
  { "Barnacle Bulwark", SH_BULWARK, 0, D_DROWNED, 0, U_NORMAL, FX_GUARD, 12, 0, 0 },
  { "Eelskin Treads", SH_TREADS, 0, D_DROWNED, 0, U_NORMAL, FX_STRIDE, 12, 0, 0 },
  { "Coral Barbute", SH_BARBUTE, 0, D_DROWNED, 0, U_BIG, 0, 0, 0, 0 },
  { "Siren's Coil", SH_COIL, 0, D_DROWNED, 0, U_NORMAL, FX_MAGIC, 25, 0, 0 },
  { "Leviathan Scale", SH_SCALE_MAIL, 0, D_DROWNED, 0, 170, FX_HP, 10, 0, 0 },
  { "Undertow", SH_GLAIVE, 0, D_DROWNED, 0, U_NORMAL, FX_GOLD, 40, 0, 0 },
  // ---- Ember Forge ----
  { "Colossus Maul", SH_GREAT_MAUL, 0, D_FORGE, 1, U_HUGE, 0, 0, 0, 0 },
  { "Anvilbreaker", SH_WAR_HAMMER, 0, D_FORGE, 0, U_BIG, FX_GUARD, 5, 0, 0 },
  { "Forgemaster Vest", SH_VEST, 0, D_FORGE, 0, U_NORMAL, FX_GUARD, 14, 0, 0 },
  { "Cinder Sabatons", SH_SABATONS, 0, D_FORGE, 0, U_NORMAL, FX_TRAP, 80, 0, 0 },
  { "Imp Tongue", SH_KRIS, 0, D_FORGE, 0, U_NORMAL, FX_LEECH, 5, 0, 0 },
  { "Slag Medallion", SH_MEDALLION, 0, D_FORGE, 0, U_NORMAL, FX_XP, 20, 0, 0 },
  { "Molten Knuckles", SH_KNUCKLES, 0, D_FORGE, 0, U_BIG, 0, 0, 0, 0 },
  { "Golem Seal", SH_SEAL, 0, D_FORGE, 0, U_NORMAL, FX_HP, 12, 0, 0 },
  // ---- Frost Spire ----
  { "Wyrmfrost", SH_CLAYMORE, 0, D_FROST, 1, U_HUGE, 0, 0, 0, 0 },
  { "Wolfpelt Mantle", SH_MANTLE, 0, D_FROST, 0, U_NORMAL, FX_STRIDE, 15, 0, 0 },
  { "Trollblood Cuirass", SH_CUIRASS, 0, D_FROST, 0, U_NORMAL, FX_LEECH, 6, 0, 0 },
  { "Yeti Mittens", SH_MITTENS, 0, D_FROST, 0, U_NORMAL, FX_GUARD, 15, 0, 0 },
  { "Icicle", SH_RAPIER, 0, D_FROST, 0, U_BIG, 0, 0, 0, 0 },
  { "Winter's Eye", SH_EYE_PENDANT, 0, D_FROST, 0, U_NORMAL, FX_MAGIC, 35, 0, 0 },
  { "Avalanche", SH_TOWER_SHIELD, 0, D_FROST, 0, U_HUGE, 0, 0, 0, 0 },
  { "Frostbite Band", SH_BAND, 0, D_FROST, 0, U_NORMAL, FX_XP, 25, 0, 0 },
  // ---- Abyssal Gate ----
  { "Doomcrown", SH_CROWN, 0, D_ABYSS, 1, U_BIG, FX_ALL, 10, FX_XP, 20 },
  { "Hellhound Collar", SH_CHOKER, 0, D_ABYSS, 0, U_NORMAL, FX_LEECH, 8, 0, 0 },
  { "Fiendfang", SH_KATAR, 0, D_ABYSS, 0, U_HUGE, 0, 0, 0, 0 },
  { "Soulreaver", SH_SCYTHE, 0, D_ABYSS, 0, U_NORMAL, FX_LEECH, 5, FX_XP, 15 },
  { "Gatewarden", SH_FULL_PLATE, 0, D_ABYSS, 0, U_NORMAL, FX_GUARD, 20, 0, 0 },
  { "Brimstone Ring", SH_SUN_RING, 0, D_ABYSS, 0, U_NORMAL, FX_GOLD, 60, 0, 0 },
  { "Ashwalk Greaves", SH_GREAVES, 0, D_ABYSS, 0, U_NORMAL, FX_STRIDE, 20, 0, 0 },
  { "Last Vigil", SH_GREAT_STAFF, 0, D_ABYSS, 0, U_NORMAL, FX_MAGIC, 50, 0, 0 },

  // ---- セット装備 ----
  { "Scout Hood", SH_HOOD, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0 },
  { "Scout Knife", SH_KNIFE, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0 },
  { "Scout Shoes", SH_SHOES, 1, D_CELLAR, 0, SET_PART, 0, 0, 0, 0 },
  { "Raider Cap", SH_CAP, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0 },
  { "Raider Hatchet", SH_HATCHET, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0 },
  { "Raider Targe", SH_TARGE, 2, D_WARREN, 0, SET_PART, 0, 0, 0, 0 },
  { "Grave Coif", SH_COIF, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0 },
  { "Grave Mail", SH_RING_MAIL, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0 },
  { "Grave Band", SH_BAND, 3, D_CRYPT, 0, SET_PART, 0, 0, 0, 0 },
  { "Spore Mask", SH_MASK, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0 },
  { "Spore Robe", SH_ROBE, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0 },
  { "Spore Wand", SH_WAND, 4, D_FUNGAL, 0, SET_PART, 0, 0, 0, 0 },
  { "Tide Sallet", SH_SALLET, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0 },
  { "Tide Scale", SH_SCALE_MAIL, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0 },
  { "Tide Pike", SH_PIKE, 5, D_DROWNED, 0, SET_PART, 0, 0, 0, 0 },
  { "Anvil Helm", SH_GREAT_HELM, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0 },
  { "Anvil Plate", SH_BREASTPLATE, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0 },
  { "Anvil Fists", SH_GAUNTLETS, 6, D_FORGE, 0, SET_PART, 0, 0, 0, 0 },
  { "Slayer Spear", SH_SPEAR, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Slayer Bracers", SH_VAMBRACES, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Slayer Striders", SH_STRIDERS, 7, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Court Circlet", SH_CIRCLET, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Court Mantle", SH_MANTLE, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Court Signet", SH_SIGNET, 8, D_FROST, 0, SET_PART, 0, 0, 0, 0 },
  { "Hellforge Maul", SH_MAUL, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0 },
  { "Hellforge Helm", SH_HORNED_HELM, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0 },
  { "Hellforge Plate", SH_FULL_PLATE, 9, D_ABYSS, 0, SET_PART, 0, 0, 0, 0 },
  { "Pilgrim Staff", SH_STAFF, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Pilgrim Cloak", SH_CLOAK, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Pilgrim Sandals", SH_SANDALS, 10, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Seeker Gloves", SH_GLOVES, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Seeker Locket", SH_LOCKET, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Seeker Loop", SH_LOOP, 11, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Sentinel Bulwark", SH_BULWARK, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Sentinel Greaves", SH_GREAVES, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
  { "Sentinel Torc", SH_TORC, 12, ANY_DUNGEON, 0, SET_PART, 0, 0, 0, 0 },
};
const int g_special_count = sizeof(g_specials) / sizeof(g_specials[0]);

// 3部位の効果は、2部位の効果に加えてつく（Pilgrim は合計で Stride +25%）
const SetDef g_sets[] = {
  { "Cellar Scout", FX_STRIDE, 5, FX_GOLD, 20 },
  { "Goblin Raider", FX_GOLD, 15, FX_MAGIC, 15 },
  { "Gravewalker", FX_HP, 10, FX_LEECH, 4 },
  { "Mycologist", FX_POTION, 30, FX_XP, 15 },
  { "Tidebound", FX_GUARD, 8, FX_STRIDE, 10 },
  { "Anvilborn", FX_DEF, 15, FX_GUARD, 15 },
  { "Wyrmslayer", FX_ATK, 15, FX_MAGIC, 30 },
  { "Winter Court", FX_XP, 20, FX_GOLD, 40 },
  { "Hellforge", FX_ATK, 20, FX_LEECH, 8 },
  { "Pilgrim", FX_STRIDE, 10, FX_STRIDE, 15 },
  { "Seeker", FX_GOLD, 25, FX_MAGIC, 30 },
  { "Sentinel", FX_GUARD, 10, FX_HP, 20 },
};
const int g_set_count = sizeof(g_sets) / sizeof(g_sets[0]);
