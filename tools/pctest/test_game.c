// ============================================================
// ゲームロジックの PC テスト
//   game.c をそのまま取り込んで、内部の関数（make_drop_item など）も試す。
//   使い方: bash tools/pctest/run.sh
// ============================================================
#include "pebble.h"

int32_t pctest_steps_today = 0;
int32_t pctest_day_total = 0;
uint8_t pctest_persist[PCTEST_PERSIST_KEYS][PCTEST_PERSIST_SIZE];
int pctest_persist_len[PCTEST_PERSIST_KEYS];

#include "../../src/c/game.c"
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
  Item rare = make_drop_item(20, 5);
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

static void test_sets(void) {
  puts("Set bonuses");
  reset_game(11);
  // 同じセットの2つを装備する
  int first = -1, second = -1;
  for (int i = 0; i < g_special_count; i++) {
    if (!g_specials[i].set_id) continue;
    if (first < 0) first = i;
    else if (g_specials[i].set_id == g_specials[first].set_id && g_specials[i].base != g_specials[first].base) {
      second = i;
      break;
    }
  }
  check(first >= 0 && second >= 0, "the table has a set with two pieces");
  Item a = make_item(g_specials[first].base, 30);
  a.rarity = RARITY_SET;
  a.flags = ITEM_FLAG_IDENTIFIED | (uint8_t)(first << ITEM_SPECIAL_SHIFT);
  memset(s_equip, 0, sizeof(s_equip));
  s_equip[g_bases[g_specials[first].base].slot] = a;
  ItemStats alone = game_item_stats(&a);

  Item b = make_item(g_specials[second].base, 30);
  b.rarity = RARITY_SET;
  b.flags = ITEM_FLAG_IDENTIFIED | (uint8_t)(second << ITEM_SPECIAL_SHIFT);
  s_equip[g_bases[g_specials[second].base].slot] = b;
  ItemStats paired = game_item_stats(&a);
  printf("  1 piece: ATK+%d DEF+%d HP+%d -> 2 pieces: ATK+%d DEF+%d HP+%d\n",
         alone.atk, alone.def, alone.hp, paired.atk, paired.def, paired.hp);
  check(game_set_pieces_equipped(&a) == 2, "set pieces are counted");
  check(paired.atk + paired.def + paired.hp > alone.atk + alone.def + alone.hp, "set bonus applies");
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
  for (int i = 0; i < STASH_SIZE; i++) s_stash[i] = make_item(i % g_base_count, 1 + i);
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
  check(game_codex_seen(0) && game_codex_seen(3), "starting gear is in the codex");
  int before = game_codex_seen_count();

  Item amulet = make_item(6, 5);
  bag_add(&amulet);
  check(game_codex_seen(6) && game_codex_seen_count() == before + 1, "picking up an item records it");

  // 固有装備は鑑定するまで名前が載らない
  int special = -1;
  for (int i = 0; i < g_special_count; i++) if (!g_specials[i].set_id) { special = i; break; }
  Item uniq = make_item(g_specials[special].base, 30);
  uniq.rarity = RARITY_UNIQUE;
  uniq.flags = (uint8_t)(special << ITEM_SPECIAL_SHIFT);   // 未鑑定
  memset(s_bag, 0, sizeof(s_bag));
  bag_add(&uniq);
  check(!game_codex_seen(g_base_count + special), "an unidentified unique stays ???");
  s_hero.scrolls_id = 1;
  game_identify_with_scroll(0);
  check(game_codex_seen(g_base_count + special), "identifying reveals it");

  game_save();
  memset(s_codex, 0, sizeof(s_codex));
  game_init();
  check(game_codex_seen(g_base_count + special), "the codex survives a reload");

  // 図鑑のない古いセーブは、持ち物から作り直す
  persist_delete(KEY_CODEX);
  game_init();
  // 装備（初期装備）と持ち物（鑑定済みの固有装備）からは載り、手放したお守りは載らない
  check(game_codex_seen(0) && game_codex_seen(g_base_count + special) && !game_codex_seen(6),
        "old saves rebuild the codex from what is carried");
  printf("  codex: %d / %d entries\n", game_codex_seen_count(), game_codex_size());

  // 保存領域の合計（Pebble は約4KB まで）
  int total = 0;
  for (int k = 0; k < PCTEST_PERSIST_KEYS; k++) total += pctest_persist_len[k];
  printf("  persist storage used: %d bytes\n", total);
  check(total < 4000, "save data fits in 4KB");
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
    if (game_run_mode() == RUN_NONE && game_depart(pick)) runs++;
    int before_deaths = s_hero.deaths;
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
  test_rarity_distribution();
  test_affixes();
  test_stats_and_price();
  test_identify();
  test_sets();
  test_return_trip();
  test_stash();
  test_blacksmith();
  test_codex();
  test_play_balance();
  printf("\n%s (%d failure%s)\n", s_failures ? "FAILED" : "ALL PASSED", s_failures,
         s_failures == 1 ? "" : "s");
  return s_failures ? 1 : 0;
}
