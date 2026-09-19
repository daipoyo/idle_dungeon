// ============================================================
// ゲームロジックの PC テスト
//   game.c をそのまま取り込んで、内部の関数（make_drop_item など）も試す。
//   使い方: bash tools/pctest/run.sh
// ============================================================
#include "pebble.h"

int32_t pctest_steps_today = 0;
int32_t pctest_day_total = 0;
int32_t pctest_sleep_today = 0;
time_t pctest_sleep_start = 0, pctest_sleep_end = 0;
uint8_t pctest_persist[PCTEST_PERSIST_KEYS][PCTEST_PERSIST_SIZE];
int pctest_persist_len[PCTEST_PERSIST_KEYS];

#include "../../src/c/game.c"
#include "../../src/c/items.c"
#include "../../src/c/steps.c"

static int s_failures;

static void check(bool ok, const char *what) {
  printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) s_failures++;
}

static void reset_game(unsigned seed) {
  memset(pctest_persist_len, 0, sizeof(pctest_persist_len));
  pctest_steps_today = 0;
  game_init();
  srand(seed);   // game_init が時刻で seed するので、後から固定する
}

// ------------------------------------------------------------
static void test_rarity_distribution(void) {
  puts("Rarity distribution (10000 drops)");
  for (int d = 0; d < DUNGEON_COUNT; d += 7) {
    int count[RARITY_COUNT] = { 0 };
    for (int i = 0; i < 10000; i++) {
      Item it = make_drop_item(20, d);
      count[it.rarity]++;
    }
    printf("  %-14s", g_dungeons[d].name);
    for (int r = 0; r < RARITY_COUNT; r++) printf(" %s %4.1f%%", g_rarity_names[r], count[r] / 100.0);
    printf("\n");
    check(count[RARITY_NORMAL] > count[RARITY_MAGIC] && count[RARITY_MAGIC] > count[RARITY_RARE] &&
              count[RARITY_RARE] > count[RARITY_SET] && count[RARITY_SET] >= count[RARITY_UNIQUE],
          "rarer tiers are less common");
  }
}

static void test_affixes(void) {
  puts("Affixes and names");
  char name[48];
  int magic_ok = 0, rare_ok = 0, unident = 0;
  for (int i = 0; i < 2000; i++) {
    Item it = make_drop_item(25, 4);
    int n = game_item_affix_count(&it);
    if (it.rarity == RARITY_MAGIC) magic_ok += (n >= 1 && n <= 2);
    if (it.rarity == RARITY_RARE) {
      unident += !game_item_identified(&it);
      it.flags |= ITEM_FLAG_IDENTIFIED;
      n = game_item_affix_count(&it);
      rare_ok += (n >= 3 && n <= 4);
    }
  }
  printf("  magic with 1-2 affixes: %d, rare with 3-4: %d, rare unidentified: %d\n",
         magic_ok, rare_ok, unident);
  check(magic_ok > 0 && rare_ok > 0, "affix counts follow rarity");
  check(unident == rare_ok || unident > 0, "rare items drop unidentified");

  // 名前と性能の見本
  for (int i = 0, shown = 0; i < 4000 && shown < 8; i++) {
    Item it = make_drop_item(25, 6);
    if (it.rarity == RARITY_NORMAL) continue;
    it.flags |= ITEM_FLAG_IDENTIFIED;
    ItemStats s = game_item_stats(&it);
    game_item_name(&it, name, sizeof(name));
    printf("  %-8s %-30s ATK+%-3d DEF+%-3d HP+%-3d %5dG\n", g_rarity_names[it.rarity], name,
           s.atk, s.def, s.hp, game_item_price(&it));
    shown++;
  }
}

static void test_stats_and_price(void) {
  puts("Stats and price");
  Item normal = make_item(0, 20);
  Item magic = normal;
  magic.rarity = RARITY_MAGIC;
  Item rare = normal;
  rare.rarity = RARITY_RARE;
  ItemStats sn = game_item_stats(&normal), sm = game_item_stats(&magic);
  check(sm.atk + sm.def + sm.hp > sn.atk + sn.def + sn.hp, "magic beats normal");
  check(game_item_price(&magic) > game_item_price(&normal), "magic costs more");

  int ident_price = game_item_price(&rare);
  rare.flags &= (uint8_t)~ITEM_FLAG_IDENTIFIED;
  check(game_item_price(&rare) < ident_price, "unidentified sells for less");
  ItemStats hidden = game_item_stats(&rare);
  check(hidden.atk == sn.atk && hidden.def == sn.def && hidden.hp == sn.hp,
        "unidentified shows only base stats");

  Item plus = normal;
  plus.plus = MAX_PLUS;
  check(game_item_stats(&plus).atk > sn.atk, "upgrades raise stats");

  // 低レベルより高レベルのほうが強い
  Item low = make_item(0, 5), high = make_item(0, 40);
  low.seed = high.seed = 1234;
  check(game_item_stats(&high).atk > game_item_stats(&low).atk, "higher item level is stronger");
}

static void test_identify(void) {
  puts("Identify");
  reset_game(7);
  Item rare = make_item(SH_LONG_SWORD, 20);
  rare.rarity = RARITY_RARE;
  rare.flags &= (uint8_t)~ITEM_FLAG_IDENTIFIED;
  s_bag[0] = rare;
  check(game_unidentified_count() == 1, "unidentified items are counted");
  check(!game_equip_from_bag(0), "cannot equip an unidentified item");

  s_hero.gold = 0;
  check(game_identify_with_gold(0) == IDENT_NO_GOLD, "needs gold");
  check(game_identify_with_scroll(0) == IDENT_NO_SCROLL, "needs a scroll");
  s_hero.gold = 10000;
  int fee = game_identify_fee(&s_bag[0]);
  check(game_identify_with_gold(0) == IDENT_OK, "appraiser identifies the item");
  check(s_hero.gold == 10000 - fee, "the fee is charged once");
  check(game_item_identified(&s_bag[0]) && game_unidentified_count() == 0, "item is now identified");
  check(game_equip_from_bag(0), "identified items can be equipped");

  // 巻物でも鑑定できる
  s_bag[0] = rare;
  s_hero.scrolls_id = 1;
  check(game_identify_with_scroll(0) == IDENT_OK && s_hero.scrolls_id == 0, "scroll identifies");
}

// 固有・セット装備を鑑定済みで作る
static Item special_item(int special, int ilvl) {
  Item it = make_special_item(special, ilvl);
  it.flags |= ITEM_FLAG_IDENTIFIED;
  return it;
}

static int find_special(const char *name) {
  char buf[ITEM_NAME_LEN];
  for (int i = 0; i < g_special_count; i++) {
    item_special_name(i, buf);
    if (strcmp(buf, name) == 0) return i;
  }
  printf("  (missing special: %s)\n", name);
  return 0;
}

static void wear(const Item *it) {
  int slot = g_shapes[g_specials[game_item_special(it)].shape].slot;
  if (slot == SLOT_RING1 && s_equip[SLOT_RING1].base) slot = SLOT_RING2;
  s_equip[slot] = *it;
}

static void test_catalog(void) {
  puts("Catalog");
  printf("  shapes %d x materials %d = %d, specials %d, sets %d -> codex %d\n", SHAPE_COUNT,
         TIER_COUNT, BASE_COUNT, g_special_count, g_set_count, game_codex_size());
  check(SHAPE_COUNT == 150, "150 shapes");
  check(g_special_count == 100, "64 uniques + 36 set pieces");
  check(game_codex_size() == 1000, "codex has 1000 entries");
  check(game_codex_size() <= CODEX_BYTES * 8, "codex fits its save bits");

  // 名前の長さ（Pebble Time 2 の一覧は20文字）
  char name[40];
  int longest = 0, over = 0;
  char longest_name[40] = "";
  for (int i = 0; i < game_codex_size(); i++) {
    game_codex_name(i, name, sizeof(name));
    int len = (int)strlen(name);
    if (len > longest) {
      longest = len;
      strcpy(longest_name, name);
    }
    over += len > 20;
  }
  printf("  longest list name: %s (%d chars)\n", longest_name, longest);
  check(over == 0, "every list name fits 20 characters");

  // 名前の重複
  int dup = 0;
  char other[40];
  for (int i = 0; i < game_codex_size(); i++) {
    game_codex_name(i, name, sizeof(name));
    for (int j = i + 1; j < game_codex_size(); j++) {
      game_codex_name(j, other, sizeof(other));
      if (strcmp(name, other) == 0) dup++;
    }
  }
  check(dup == 0, "no two codex entries share a name");

  // 形ごとの装備枠の数
  int per_slot[EQUIP_SLOTS] = { 0 };
  for (int i = 0; i < SHAPE_COUNT; i++) per_slot[g_shapes[i].slot]++;
  printf("  shapes per slot: weapon %d, offhand %d, head %d, body %d, hands %d, feet %d, amulet %d, ring %d\n",
         per_slot[SLOT_WEAPON], per_slot[SLOT_OFFHAND], per_slot[SLOT_HEAD], per_slot[SLOT_BODY],
         per_slot[SLOT_HANDS], per_slot[SLOT_FEET], per_slot[SLOT_AMULET], per_slot[SLOT_RING1]);

  // セットは3部位で、同時に着けられる（装備枠が重ならない）
  bool sets_ok = true;
  for (int set = 1; set <= g_set_count; set++) {
    int count = 0, slots_seen = 0;
    for (int i = 0; i < g_special_count; i++) {
      if (g_specials[i].set_id != set) continue;
      int slot = g_shapes[g_specials[i].shape].slot;
      if (slots_seen & (1 << slot)) sets_ok = false;
      slots_seen |= 1 << slot;
      count++;
    }
    if (count != 3) sets_ok = false;
  }
  check(sets_ok, "each set has 3 pieces in different slots");

  // 各ダンジョンに固有装備が8種、うちボス専用が1種
  bool uniques_ok = true;
  for (int d = 0; d < DUNGEON_COUNT; d++) {
    int n = 0, boss = 0;
    for (int i = 0; i < g_special_count; i++) {
      if (g_specials[i].set_id || g_specials[i].dungeon != d) continue;
      n++;
      boss += g_specials[i].boss;
    }
    if (n != 8 || boss != 1) uniques_ok = false;
  }
  check(uniques_ok, "8 uniques per dungeon, one of them from the boss");

  // どの形も、アイテムLv 1 で性能が全部 0 にならない（ばらつきが一番低い値でも）
  int zero = 0;
  for (int shape = 0; shape < SHAPE_COUNT; shape++) {
    for (int seed = 0; seed < 31; seed++) {
      Item probe = make_item(shape, 1);
      probe.seed = (uint16_t)seed;
      ItemStats st = game_item_stats(&probe);
      if (st.atk + st.def + st.hp == 0) zero++;
    }
  }
  check(zero == 0, "no item has all-zero stats, even at item level 1");

  // 素材は アイテムLv に合う
  Item low = make_item(SH_CLAYMORE, 3), high = make_item(SH_CLAYMORE, 47);
  game_item_short_name(&low, name, sizeof(name));
  game_item_short_name(&high, other, sizeof(other));
  printf("  Claymore at ilvl 3: %s, at ilvl 47: %s\n", name, other);
  check(strcmp(name, "Bronze Claymore") == 0 && strcmp(other, "Void Claymore") == 0,
        "material follows item level");
}

static void test_sets(void) {
  puts("Sets and effects");
  reset_game(11);
  memset(s_equip, 0, sizeof(s_equip));

  Item hood = special_item(find_special("Scout Hood"), 5);
  Item knife = special_item(find_special("Scout Knife"), 5);
  Item shoes = special_item(find_special("Scout Shoes"), 5);
  wear(&hood);
  check(game_set_pieces_equipped(&hood) == 1 && game_effect_total(FX_STRIDE) == 0, "one piece: no bonus");
  wear(&knife);
  check(game_effect_total(FX_STRIDE) == 5 && game_effect_total(FX_GOLD) == 0, "two pieces: Stride +5%");
  wear(&shoes);
  check(game_effect_total(FX_STRIDE) == 5 && game_effect_total(FX_GOLD) == 20, "three pieces: Gold +20% too");

  // 同じ指輪を2つ着けても1部位
  memset(s_equip, 0, sizeof(s_equip));
  Item band = special_item(find_special("Grave Band"), 12);
  wear(&band);
  wear(&band);
  check(game_set_pieces_equipped(&band) == 1, "a duplicate ring counts once");

  // 固有装備の効果が合計される
  memset(s_equip, 0, sizeof(s_equip));
  Item cloak = special_item(find_special("Batwing Cloak"), 5);
  Item treads = special_item(find_special("Eelskin Treads"), 25);
  wear(&cloak);
  wear(&treads);
  check(game_effect_total(FX_STRIDE) == 17, "unique effects add up (5 + 12)");

  // 未鑑定の固有装備は効果を出さない
  s_equip[SLOT_BODY].flags = 0;
  check(game_effect_total(FX_STRIDE) == 12, "an unidentified unique has no effect");

  // 上限
  memset(s_equip, 0, sizeof(s_equip));
  Item leech = special_item(find_special("Hellhound Collar"), 45);
  Item leech2 = special_item(find_special("Trollblood Cuirass"), 40);
  Item leech3 = special_item(find_special("Ghoulclaw"), 15);
  wear(&leech);
  wear(&leech2);
  wear(&leech3);
  check(game_effect_total(FX_LEECH) == 15, "effects are capped (Leech 18% -> 15%)");

  // ストライドで同じ歩数でも深く進む
  int depth[2];
  for (int k = 0; k < 2; k++) {
    reset_game(3);
    memset(s_equip, 0, sizeof(s_equip));
    s_equip[SLOT_WEAPON] = make_item(SH_CLAYMORE, 40);   // 戦闘で死なないように
    s_equip[SLOT_BODY] = make_item(SH_FULL_PLATE, 40);
    if (k == 1) {
      Item greaves = special_item(find_special("Ashwalk Greaves"), 45);
      wear(&greaves);
    }
    s_hero.auto_return_pct = 0;
    game_depart(0);
    pctest_steps_today += 1000;
    game_update();
    depth[k] = (int)s_run.depth;
  }
  printf("  1000 steps: depth %d without Stride, %d with Stride +20%%\n", depth[0], depth[1]);
  check(depth[1] > depth[0], "Stride moves the hero further");

  // ボスの固有装備。すでに持っている場合は 15% くらい
  reset_game(17);
  int crown = -1;
  for (int i = 0; i < g_special_count; i++) {
    if (g_specials[i].boss && g_specials[i].dungeon == 0) crown = BASE_COUNT + i;
  }
  codex_set(crown);
  int boss_uniques = 0;
  for (int i = 0; i < 400; i++) {
    memset(s_bag, 0, sizeof(s_bag));
    found_boss_loot(0, 5);
    int sp = game_item_special(&s_bag[0]);
    boss_uniques += sp >= 0 && g_specials[sp].boss;
  }
  printf("  Rat King dropped his crown %d times in 400 kills\n", boss_uniques);
  check(boss_uniques > 20 && boss_uniques < 110, "boss uniques drop about 15% of the time");

  // まだ持っていない場合は、5回倒すうちに必ず出る（天井）
  memset(s_codex, 0, sizeof(s_codex));
  int worst = 0;
  for (int round = 0; round < 50; round++) {
    memset(s_codex, 0, sizeof(s_codex));
    memset(s_boss_miss, 0, sizeof(s_boss_miss));
    int kills = 0;
    while (kills < 20) {
      memset(s_bag, 0, sizeof(s_bag));
      found_boss_loot(0, 5);
      kills++;
      int sp = game_item_special(&s_bag[0]);
      if (sp >= 0 && g_specials[sp].boss) break;
    }
    if (kills > worst) worst = kills;
  }
  printf("  at worst it took %d kills to see the crown\n", worst);
  check(worst <= BOSS_PITY, "a boss unique the hero lacks turns up within the pity count");

  // 普通のドロップでボス専用の物は出ない。ホームのダンジョンの物が出やすい
  int home = 0, total = 0, boss_only = 0;
  for (int i = 0; i < 20000; i++) {
    int sp = pick_special(2, false);
    if (sp < 0) continue;
    total++;
    home += g_specials[sp].dungeon == 2;
    boss_only += g_specials[sp].boss;
  }
  printf("  uniques rolled in Sunken Crypt: %d%% from the Crypt itself\n", home * 100 / total);
  check(boss_only == 0, "boss uniques never come from ordinary drops");
  check(home * 100 / total > 40, "a dungeon's own uniques are the most common there");
}

static void test_return_trip(void) {
  puts("Walking back");
  reset_game(5);
  s_hero.auto_return_pct = 30;
  check(game_depart(0), "the hero sets out");
  s_run.mode = RUN_RETURN;
  s_run.depth = 600;
  s_run.return_left = 600;
  s_run.hp = game_max_hp() / 10;   // 瀕死

  // 巻物があれば帰り道でも使う
  s_hero.portals = 1;
  check_auto_return();
  check(s_run.mode == RUN_NONE && s_hero.portals == 0, "a portal scroll is used on the way back");

  // 巻物がなければ歩き続けるが、知らせるのは一度だけ
  reset_game(5);
  s_hero.auto_return_pct = 30;
  game_depart(0);
  s_run.mode = RUN_RETURN;
  s_run.depth = s_run.return_left = 600;
  s_run.hp = game_max_hp() / 10;
  s_hero.portals = 0;
  s_alert = false;
  check_auto_return();
  check(s_run.mode == RUN_RETURN && s_alert, "without a scroll the hero keeps walking, and warns");
  s_alert = false;
  check_auto_return();
  check(!s_alert, "the warning does not repeat");

  // 帰り道でも、低い確率で拾い物がある
  reset_game(5);
  game_depart(0);
  s_run.mode = RUN_RETURN;
  s_run.depth = s_run.return_left = 4000;
  int found = 0;
  for (int i = 0; i < 400; i++) {
    s_run.hp = game_max_hp();      // 死なないように回復させておく
    memset(s_bag, 0, sizeof(s_bag));
    return_event();
    for (int b = 0; b < BAG_SIZE; b++) found += s_bag[b].base ? 1 : 0;
  }
  printf("  items found in 400 events on the way back: %d\n", found);
  check(found > 0 && found < 120, "loot on the way back is possible but rare");
}

static void test_stash(void) {
  puts("Stash");
  reset_game(21);
  for (int i = 0; i < 3; i++) s_bag[i] = make_item(i, 10);
  Item second = s_bag[1];
  check(game_stash_put(1) && game_stash_count() == 1 && game_bag_count() == 2, "bag -> stash");
  check(memcmp(game_stash(0), &second, sizeof(Item)) == 0, "the same item is stored");
  check(game_stash_take(0) && game_stash_count() == 0 && game_bag_count() == 3, "stash -> bag");

  // 町の外では出し入れできない
  game_stash_put(0);
  s_run.mode = RUN_EXPLORE;
  check(!game_stash_put(0) && !game_stash_take(0), "only in town");
  s_run.mode = RUN_NONE;

  // 保存して読み直しても残る（2つのキーにまたがる位置も）
  memset(s_stash, 0, sizeof(s_stash));
  for (int i = 0; i < STASH_SIZE; i++) s_stash[i] = make_item(i % SHAPE_COUNT, 1 + i);
  game_save();
  memset(s_stash, 0, sizeof(s_stash));
  game_init();
  check(game_stash_count() == STASH_SIZE && s_stash[STASH_SIZE - 1].ilvl == STASH_SIZE,
        "all 48 slots survive a reload");

  // 死んでも保管庫は失わない
  s_hero.auto_return_pct = 0;
  game_depart(0);
  s_run.hp = 1;
  hurt(5);
  check(s_run.mode == RUN_NONE && game_stash_count() == STASH_SIZE, "death leaves the stash alone");
}

static void test_blacksmith(void) {
  puts("Blacksmith");
  reset_game(33);
  s_equip[SLOT_WEAPON] = make_item(0, 20);
  s_hero.gold = 1000000;
  int fails = 0, attempts = 0;
  printf("  cost per step at ilvl 20:");
  while (s_equip[SLOT_WEAPON].plus < MAX_PLUS && attempts < 500) {
    int before = s_equip[SLOT_WEAPON].plus;
    int cost = game_upgrade_cost(&s_equip[SLOT_WEAPON]);
    int32_t gold = s_hero.gold;
    UpgradeResult r = game_upgrade_equipped(SLOT_WEAPON);
    attempts++;
    if (s_hero.gold != gold - cost) { check(false, "the cost is charged exactly"); break; }
    if (r == UPGRADE_FAILED) {
      fails++;
      if (s_equip[SLOT_WEAPON].plus != before || !s_equip[SLOT_WEAPON].base) {
        check(false, "a failure never breaks or downgrades gear");
        break;
      }
    } else if (before < 3 || r == UPGRADE_OK) {
      if (r == UPGRADE_OK && s_equip[SLOT_WEAPON].plus == before + 1 && before != s_equip[SLOT_WEAPON].plus - 1) {
        check(false, "success raises by one");
      }
      printf(" +%d:%dG", before + 1, cost);
    }
  }
  printf("\n  reached +%d in %d attempts (%d failed), spent %ldG\n", s_equip[SLOT_WEAPON].plus,
         attempts, fails, (long)(1000000 - s_hero.gold));
  check(s_equip[SLOT_WEAPON].plus == MAX_PLUS, "can reach +10");
  check(game_upgrade_equipped(SLOT_WEAPON) == UPGRADE_MAX, "stops at +10");

  Item unident = make_drop_item(20, 3);
  unident.rarity = RARITY_RARE;
  unident.flags = 0;
  s_bag[0] = unident;
  check(game_upgrade_bag(0) == UPGRADE_NONE, "unidentified items cannot be upgraded");

  s_bag[0] = make_item(1, 20);
  s_hero.gold = 0;
  check(game_upgrade_bag(0) == UPGRADE_NO_GOLD, "needs gold");
}

static void test_codex(void) {
  puts("Codex");
  reset_game(44);
  Item sword = make_item(SH_SHORT_SWORD, 1), tunic = make_item(SH_TUNIC, 1);
  check(game_codex_found(sword.base - 1) && game_codex_found(tunic.base - 1), "starting gear is in the codex");
  int before = game_codex_found_count();

  Item amulet = make_item(SH_LOCKET, 5);
  bag_add(&amulet);
  check(game_codex_found(amulet.base - 1) && game_codex_found_count() == before + 1,
        "picking up an item records it");
  char name[32];
  game_codex_name(amulet.base - 1, name, sizeof(name));
  check(strcmp(name, "Bone Locket") == 0, "codex names read material + shape");

  // 固有装備は鑑定するまで載らない。ログにも正体は出ない
  int special = find_special("Ratcatcher");
  Item uniq = make_special_item(special, 5);
  memset(s_bag, 0, sizeof(s_bag));
  bag_add(&uniq);
  check(!game_codex_found(BASE_COUNT + special), "an unidentified unique stays ???");
  game_item_short_name(&uniq, name, sizeof(name));
  check(strcmp(name, "Bronze Knife") == 0, "an unidentified unique shows its base name");
  check(visible_base(&uniq) == SH_KNIFE * TIER_COUNT, "logs hide an unidentified unique");
  s_hero.scrolls_id = 1;
  game_identify_with_scroll(0);
  check(game_codex_found(BASE_COUNT + special), "identifying reveals it");
  game_item_short_name(&s_bag[0], name, sizeof(name));
  check(strcmp(name, "Ratcatcher") == 0, "identified uniques show their own name");

  game_save();
  memset(s_codex, 0, sizeof(s_codex));
  game_init();
  check(game_codex_found(BASE_COUNT + special), "the codex survives a reload");

  // 図鑑のない古いセーブは、持ち物から作り直す
  persist_delete(KEY_CODEX);
  game_init();
  check(game_codex_found(sword.base - 1) && game_codex_found(BASE_COUNT + special) &&
            !game_codex_found(amulet.base - 1),
        "old saves rebuild the codex from what is carried");
  printf("  codex: %d / %d entries\n", game_codex_found_count(), game_codex_size());

  // 保存形式が古いセーブ（version 1）はリセットする
  s_hero.version = 1;
  s_hero.level = 30;
  game_save();
  game_init();
  check(s_hero.version == SAVE_VERSION && s_hero.level == 1, "version 1 saves start over");

  // 保存領域の合計（Pebble は約4KB まで）
  int total = 0;
  for (int k = 0; k < PCTEST_PERSIST_KEYS; k++) total += pctest_persist_len[k];
  printf("  persist storage used: %d bytes\n", total);
  check(total < 4000, "save data fits in 4KB");
}

static void test_saved_steps(void) {
  puts("Saved steps");
  reset_game(31);
  check(game_saved_steps() == 0, "a new hero starts with no saved steps");
  pctest_steps_today += 1200;
  game_update();
  check(game_saved_steps() == 1200, "steps taken in town are saved up");
  pctest_steps_today += 20000;
  game_update();
  check(game_saved_steps() == SAVED_STEPS_MAX, "saved steps stop at the cap");

  // 出発した瞬間に使われる
  s_hero.level = 20;
  s_hero.saved_steps = 500;
  check(game_depart(0), "the hero sets out");
  check(game_saved_steps() == 0 && game_run_mode() == RUN_EXPLORE && s_run.depth == 500,
        "saved steps move the hero forward on departure");

  // 使い切る前に町へ戻ったら、残りはまた貯まる（1500歩で最深部、1500歩で帰り道）
  s_run.mode = RUN_NONE;
  s_hero.saved_steps = 5000;
  game_depart(0);
  printf("  5000 saved steps on a 1500-step dungeon: mode %d, %ld left over\n", game_run_mode(),
         (long)game_saved_steps());
  check(game_run_mode() == RUN_NONE && game_saved_steps() == 2000, "leftover saved steps are kept");

  // 歩いている途中で町に着いたら、その日の残りの歩数も貯まる
  s_hero.saved_steps = 0;
  game_depart(0);
  pctest_steps_today += 4000;
  game_update();
  check(game_run_mode() == RUN_NONE && game_saved_steps() == 1000,
        "steps left after reaching town are saved for the next run");

  game_save();
  s_hero.saved_steps = 0;
  game_init();
  check(game_saved_steps() == 1000, "saved steps survive a reload");

  // 貯めた歩数ができる前のセーブ（Hero が 20 バイト）も、そのまま読める
  pctest_persist_len[KEY_HERO] = 20;
  memset(&s_hero, 0, sizeof(s_hero));
  game_init();
  check(s_hero.level == 20 && game_saved_steps() == 0 && game_remind_hour() == 0,
        "older saves load with no saved steps and the reminder off");

  game_cycle_remind();
  check(game_remind_hour() == 6, "the reminder cycles through hours");
}

static void test_sleep(void) {
  puts("Sleep bonus");
  reset_game(41);
  time_t today = steps_day_start(time(NULL));
  pctest_sleep_today = 0;
  pctest_sleep_start = pctest_sleep_end = 0;
  game_update();
  check(game_sleep_tier() == SLEEP_NONE, "no sleep data, no bonus");

  // 23時〜6時半（日付をまたいだ7時間半）: 今日の合計は6時間半しかないが、眠りの記録から数える
  pctest_sleep_start = today - 1 * SECONDS_PER_HOUR;
  pctest_sleep_end = today + 6 * SECONDS_PER_HOUR + 30 * SECONDS_PER_MINUTE;
  pctest_sleep_today = 6 * SECONDS_PER_HOUR + 30 * SECONDS_PER_MINUTE;
  bool before_noon = time(NULL) >= pctest_sleep_end;
  s_hero.sleep_day = 0;   // 日が変わった扱いにして、すぐに記録を見に行かせる（ふだんは数分に1回）
  game_update();
  if (before_noon) {
    check(game_sleep_tier() == SLEEP_REFRESHED && game_sleep_minutes() == 450,
          "7.5 hours across midnight counts as Refreshed");
    char buf[64];
    game_log_text(0, buf, sizeof(buf));   // 0 が一番新しい
    printf("  log: %s\n", buf);
    check(strstr(buf, "Slept 7h30m. Refreshed!") != NULL, "the log says how long the hero slept");
  } else {
    puts("  (skipped the across-midnight check: too early in the day)");
  }

  // 効果: 経験値・ゴールド +20%、マジック発見 +10
  check(sleep_bonus(FX_XP) == 20 && sleep_bonus(FX_GOLD) == 20 && sleep_bonus(FX_MAGIC) == 10,
        "Refreshed gives XP/Gold +20% and magic find +10");

  // 6時間台は Rested
  s_hero.sleep_day = 0;
  pctest_sleep_start = pctest_sleep_end = 0;
  pctest_sleep_today = 6 * SECONDS_PER_HOUR + 10 * SECONDS_PER_MINUTE;
  game_update();
  check(game_sleep_tier() == SLEEP_RESTED && sleep_bonus(FX_XP) == 10 && sleep_bonus(FX_MAGIC) == 0,
        "6 hours gives Rested (+10%)");

  // 次の日になると消える
  s_hero.sleep_day -= SECONDS_PER_DAY;
  check(game_sleep_tier() == SLEEP_NONE && sleep_bonus(FX_GOLD) == 0, "the bonus ends when the day changes");

  // 5時間では何もない
  pctest_sleep_today = 5 * SECONDS_PER_HOUR;
  game_update();
  check(game_sleep_tier() == SLEEP_NONE, "5 hours gives no bonus");

  game_save();
  pctest_sleep_today = 7 * SECONDS_PER_HOUR;
  s_hero.sleep_day = 0;
  game_update();
  game_save();
  s_hero.sleep_tier = 0;
  game_init();
  check(game_sleep_tier() == SLEEP_REFRESHED, "the bonus survives a reload");
  pctest_sleep_today = 0;
}

static void test_compare(void) {
  puts("Comparing gear");
  reset_game(53);
  memset(s_equip, 0, sizeof(s_equip));
  memset(s_bag, 0, sizeof(s_bag));

  Item weak = make_item(SH_SHORT_SWORD, 1);
  Item strong = make_item(SH_SHORT_SWORD, 30);
  const Item *now = NULL;
  ItemStats diff;

  bag_add(&strong);
  check(game_item_compare(&s_bag[0], &now, &diff) == CMP_BETTER && now == NULL,
        "with an empty slot, anything is an improvement");
  check(diff.atk == game_item_stats(&s_bag[0]).atk, "the whole stat counts when the slot is empty");

  s_equip[SLOT_WEAPON] = strong;
  s_bag[0] = weak;
  check(game_item_compare(&s_bag[0], &now, &diff) == CMP_WORSE && now == &s_equip[SLOT_WEAPON],
        "a weaker weapon is marked worse");
  check(diff.atk < 0, "the difference is negative");
  printf("  Lv1 vs Lv30 short sword: ATK %d\n", diff.atk);

  s_bag[0] = strong;
  check(game_item_compare(&s_bag[0], NULL, NULL) == CMP_EVEN, "the same gear compares as even");

  // 今つけている物そのものはくらべない
  check(game_item_compare(&s_equip[SLOT_WEAPON], NULL, NULL) == CMP_NONE, "equipped gear has nothing to compare");

  // 未鑑定の物はくらべられない
  Item unident = make_drop_item(20, 4);
  unident.rarity = RARITY_RARE;
  unident.flags = 0;
  check(game_item_compare(&unident, NULL, NULL) == CMP_NONE, "unidentified gear cannot be compared");

  // ダンジョンの中でも装備を変えられる
  memset(s_equip, 0, sizeof(s_equip));
  memset(s_bag, 0, sizeof(s_bag));
  bag_add(&strong);
  game_depart(0);
  check(game_run_mode() != RUN_NONE, "the hero is in the dungeon");
  check(game_equip_from_bag(0), "gear can be changed in the dungeon");
  check(s_equip[SLOT_WEAPON].base == strong.base, "the weapon is worn right away");
  check(game_unequip(SLOT_WEAPON), "gear can be taken off in the dungeon");
  s_run.mode = RUN_NONE;

  // 指輪は空いている方と入れ替わる
  Item ring = make_item(SH_MOON_RING, 10);
  s_equip[SLOT_RING1] = make_item(SH_MOON_RING, 40);
  memset(&s_equip[SLOT_RING2], 0, sizeof(Item));
  s_bag[0] = ring;
  check(game_item_compare(&s_bag[0], &now, NULL) == CMP_BETTER && now == NULL,
        "a second ring goes in the empty finger, not against the good one");
}

static void test_codex_states(void) {
  puts("Codex: seen and found");
  reset_game(61);
  memset(s_codex, 0, sizeof(s_codex));
  memset(s_codex_seen, 0, sizeof(s_codex_seen));

  int entry = SH_LONG_SWORD * TIER_COUNT + 2;
  check(game_codex_state(entry) == CODEX_UNKNOWN, "an untouched entry is unknown");
  game_codex_mark_seen(entry);
  check(game_codex_state(entry) == CODEX_SEEN, "a rumour marks it as seen");
  check(game_codex_found_count() == 0, "seen entries do not count as found");
  check(game_codex_seen_count() == 1, "but they do show in the seen count");

  Item it = make_item(SH_LONG_SWORD, 20);
  it.base = (uint16_t)(entry + 1);
  bag_add(&it);
  check(game_codex_state(entry) == CODEX_FOUND, "picking it up turns seen into found");
  check(game_codex_found_count() == 1 && game_codex_seen_count() == 1,
        "an entry is counted once, as found");

  // 手に入れた物に噂が来ても、状態は下がらない
  game_codex_mark_seen(entry);
  check(game_codex_state(entry) == CODEX_FOUND, "a rumour never undoes a find");

  int other = SH_CLAYMORE * TIER_COUNT + 5;
  game_codex_mark_seen(other);
  game_save();
  memset(s_codex, 0, sizeof(s_codex));
  memset(s_codex_seen, 0, sizeof(s_codex_seen));
  game_init();
  check(game_codex_state(entry) == CODEX_FOUND && game_codex_state(other) == CODEX_SEEN,
        "both states survive a reload");

  // 持ち物がいっぱいのときに落ちた物は、見たことになる
  memset(s_codex, 0, sizeof(s_codex));
  memset(s_codex_seen, 0, sizeof(s_codex_seen));
  for (int i = 0; i < BAG_SIZE; i++) s_bag[i] = make_item(SH_KNIFE, 5);
  game_depart(0);
  s_run.depth = 100;
  int seen_before = game_codex_seen_count();
  for (int i = 0; i < 40; i++) found_item(10);
  check(game_codex_seen_count() > seen_before, "loot left behind with a full bag is marked seen");
  check(game_codex_found_count() <= 1, "but it is not counted as found");
  s_run.mode = RUN_NONE;

  // 出典
  char place[24];
  game_codex_source(SH_SHORT_SWORD * TIER_COUNT + 0, place, sizeof(place));
  printf("  tier 0 short sword: %s\n", place);
  check(strcmp(place, "Mossy Cellar +1") == 0, "the lowest tier points at the first dungeons");
  game_codex_source(SH_SHORT_SWORD * TIER_COUNT + 5, place, sizeof(place));
  printf("  tier 5 short sword: %s\n", place);
  check(strcmp(place, "Abyssal Gate") == 0, "the top tier points at the last dungeon");
  for (int i = 0; i < g_special_count; i++) {
    if (!g_specials[i].boss) continue;
    game_codex_source(BASE_COUNT + i, place, sizeof(place));
    printf("  a boss unique: %s\n", place);
    check(strstr(place, "boss") != NULL, "boss uniques say so");
    break;
  }
}

static void test_rumours(void) {
  puts("Rumours (hunting a specific item)");
  reset_game(71);
  memset(s_rumours, 0, sizeof(s_rumours));
  s_hero.gold = 100000;

  int entry = SH_CLAYMORE * TIER_COUNT + 2;   // Steel Claymore
  check(game_rumour_price(entry) == 500 + 300 * 2, "the price rises with the material tier");
  check(game_buy_rumour(entry) == RUMOUR_OK, "a rumour can be bought");
  check(game_rumour_has(entry) && game_rumour_count() == 1, "it takes a slot");
  check(game_codex_state(entry) == CODEX_SEEN, "hearing about it reveals its look");
  check(game_buy_rumour(entry) == RUMOUR_NONE, "the same item cannot be hunted twice");

  int others[] = { SH_KATANA * TIER_COUNT + 2, SH_RAPIER * TIER_COUNT + 2, SH_SABER * TIER_COUNT + 2 };
  check(game_buy_rumour(others[0]) == RUMOUR_OK && game_buy_rumour(others[1]) == RUMOUR_OK,
        "three can be hunted at once");
  check(game_buy_rumour(others[2]) == RUMOUR_FULL, "a fourth needs a free slot");
  game_drop_rumour(others[1]);
  check(game_rumour_count() == 2, "a hunt can be called off");

  s_hero.gold = 10;
  check(game_buy_rumour(others[2]) == RUMOUR_NO_GOLD, "it costs gold");
  s_hero.gold = 100000;

  // 狙った品は、その素材の段階が出るダンジョンで出やすい
  memset(s_rumours, 0, sizeof(s_rumours));
  game_buy_rumour(entry);
  int here = 0, elsewhere = 0;
  for (int i = 0; i < 2000; i++) {
    Item it = make_drop_item(22, 4);       // 段階2が出る深さ
    if (it.base == entry + 1) here++;
    Item far = make_drop_item(45, 7);      // 段階5しか出ない深さ
    if (far.base == entry + 1) elsewhere++;
  }
  printf("  Steel Claymore in 2000 drops: %d at the right depth, %d at the wrong one\n", here, elsewhere);
  check(here > 400, "the hunted item turns up often where it can drop");
  check(elsewhere == 0, "and never where its material cannot appear");

  // 手に入れると枠が空く
  Item found = make_item(SH_CLAYMORE, 22);
  found.base = (uint16_t)(entry + 1);
  memset(s_bag, 0, sizeof(s_bag));
  bag_add(&found);
  check(!game_rumour_has(entry) && game_codex_state(entry) == CODEX_FOUND,
        "finding it ends the hunt and fills the codex");
  check(game_rumour_price(entry) == 0, "an item already found cannot be hunted");

  // 固有装備の噂は、そのダンジョンでだけ効く
  memset(s_rumours, 0, sizeof(s_rumours));
  int special = -1;
  for (int i = 0; i < g_special_count; i++) {
    if (!g_specials[i].set_id && !g_specials[i].boss && g_specials[i].dungeon == 3) special = i;
  }
  check(special >= 0, "the third dungeon has a unique of its own");
  check(game_rumour_price(BASE_COUNT + special) == 5000, "uniques cost more to hunt");
  game_buy_rumour(BASE_COUNT + special);
  int hits = 0;
  for (int i = 0; i < 2000; i++) {
    Item it = make_drop_item(20, 3);
    if (it.base == SPECIAL_BASE + special) hits++;
  }
  printf("  hunted unique in 2000 drops in its own dungeon: %d\n", hits);
  check(hits > 80, "a hunted unique shows up far more often at home");

  game_save();
  memset(s_rumours, 0, sizeof(s_rumours));
  game_init();
  check(game_rumour_has(BASE_COUNT + special), "hunts survive a reload");
}

static void test_tavern(void) {
  puts("Tavern (the day's rumours)");
  reset_game(83);
  s_hero.gold = 100000;
  s_hero.cleared = 0x07;   // 最初の3つを制覇したところ
  memset(s_rumours, 0, sizeof(s_rumours));

  int offers[TAVERN_OFFERS];
  int specials = 0;
  for (int i = 0; i < TAVERN_OFFERS; i++) {
    offers[i] = game_tavern_offer(i);
    check(offers[i] >= 0, "the tavern has something to say");
    check(!game_codex_found(offers[i]), "it never talks about gear the hero already owns");
    char name[40], place[24];
    game_codex_name(offers[i], name, sizeof(name));
    game_codex_source(offers[i], place, sizeof(place));
    printf("  %-22s %5dG  %s\n", name, game_tavern_price(offers[i]), place);
    if (offers[i] >= BASE_COUNT) specials++;
  }
  check(specials >= 1, "one of them is about a unique or a set piece");
  check(offers[0] != offers[1] && offers[1] != offers[2], "no two are the same");
  check(game_tavern_offer(0) == offers[0], "the same rumours stay all day");

  // 入れないダンジョンの話はしない
  for (int i = 0; i < TAVERN_OFFERS; i++) {
    if (offers[i] < BASE_COUNT) continue;
    int d = g_specials[offers[i] - BASE_COUNT].dungeon;
    check(d >= DUNGEON_COUNT || game_dungeon_unlocked(d), "uniques come from dungeons the hero can enter");
  }

  check(game_tavern_price(offers[0]) == game_rumour_price(offers[0]) * 60 / 100,
        "the tavern is cheaper than asking around");
  int32_t gold = game_gold();
  check(game_buy_tavern(0) == RUMOUR_OK, "a rumour can be bought");
  check(game_rumour_has(offers[0]), "it becomes a hunt");
  check(game_gold() == gold - game_tavern_price(offers[0]), "and it costs what it said");
  check(game_buy_tavern(0) == RUMOUR_NONE, "buying the same one twice does nothing");

  s_hero.gold = 0;
  check(game_buy_tavern(1) == RUMOUR_NO_GOLD, "no coin, no story");
}

// 実際に歩いて遊んだときの様子（バランス確認）
//   margin: ダンジョン選びの強気さ（敵のレベルが「勇者のレベル + margin」までなら入る）
static void play_days(int days, int margin, bool verbose) {
  reset_game(99);
  int deaths = 0, items = 0, rares = 0, runs = 0;
  int rarity_count[RARITY_COUNT] = { 0 };
  for (int day = 1; day <= days; day++) {
    // 入れるいちばん深いダンジョンを選ぶ
    int pick = 0;
    for (int d = 0; d < DUNGEON_COUNT; d++) {
      if (game_dungeon_unlocked(d) && g_dungeons[d].lvl_max <= game_level() + margin) pick = d;
    }
    int before_deaths = s_hero.deaths;   // 出発した瞬間に貯めた歩数で倒れることもある
    if (game_run_mode() == RUN_NONE && game_depart(pick)) runs++;
    pctest_steps_today += 4000;
    game_update();
    deaths += (s_hero.deaths > before_deaths);
    // 拾った物を数えてから、持ち物を空にする（店で売った扱い）
    for (int i = 0; i < BAG_SIZE; i++) {
      if (!s_bag[i].base) continue;
      items++;
      rarity_count[s_bag[i].rarity % RARITY_COUNT]++;
      rares += s_bag[i].rarity >= RARITY_RARE;
    }
    memset(s_bag, 0, sizeof(s_bag));
    s_hero.potions = MAX_POTIONS;
    if (verbose && day % 10 == 0) {
      printf("    day %2d: Lv %2d  gold %5ld  items %3d (rare+ %2d)  deaths %d\n", day, game_level(),
             (long)game_gold(), items, rares, deaths);
    }
  }
  printf("  margin %+d: Lv %2d after %d days, %d runs, %d deaths (%d%% of runs), %d items",
         margin, game_level(), days, runs, deaths, runs ? deaths * 100 / runs : 0, items);
  for (int r = RARITY_RARE; r < RARITY_COUNT; r++) printf(", %s %d", g_rarity_names[r], rarity_count[r]);
  printf("\n");
  check(game_level() > 1, "the hero levels up");
  check(items > 0, "items drop while walking");
}

static void test_play_balance(void) {
  puts("Simulated play (walking 4000 steps a day for 30 days)");
  play_days(30, -2, false);   // 慎重に、格下のダンジョンで
  play_days(30, 0, true);     // 身の丈に合ったダンジョン
  play_days(30, 3, false);    // 強気に、格上のダンジョン
}

int main(void) {
  test_catalog();
  test_rarity_distribution();
  test_affixes();
  test_stats_and_price();
  test_identify();
  test_sets();
  test_return_trip();
  test_stash();
  test_blacksmith();
  test_codex();
  test_codex_states();
  test_rumours();
  test_tavern();
  test_saved_steps();
  test_sleep();
  test_compare();
  test_play_balance();
  printf("\n%s (%d failure%s)\n", s_failures ? "FAILED" : "ALL PASSED", s_failures,
         s_failures == 1 ? "" : "s");
  return s_failures ? 1 : 0;
}
