// ============================================================
// 長く遊んだときの進み方を調べる
//   毎日ダンジョンに出発し、町では売って・買って・鑑定して・強い物に着替える。
//   全ダンジョン制覇までの日数と、図鑑の埋まり方を出す。
//   使い方: bash tools/pctest/sim_progress.sh [1日の歩数]
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

#define YEARS 8
#define MAX_DAYS (365 * YEARS)

// その日に入るダンジョンを選ぶ
//   まだ倒していない一番浅いダンジョンを目標にする。ただし敵が強すぎる間は、
//   入れるなかで一番深い（実力に見合う）ダンジョンで鍛える
static bool s_rotate;
static bool s_hunt;   // 噂を買って狙い撃ちする
static int s_next_dungeon;   // 制覇後は毎日ちがうダンジョンへ（図鑑集め）
static int s_day;

static int pick_dungeon(void) {
  int train = 0;
  if (s_rotate) {
    int all = 1;
    for (int d = 0; d < DUNGEON_COUNT; d++) all &= game_dungeon_cleared(d) ? 1 : 0;
    if (all) {
      // 出発するたびに次のダンジョンへ（探索は何日もかかるので、日付では順番がずれる）
      static int next;
      s_next_dungeon = next + 1;
      return next++ % DUNGEON_COUNT;
    }
  }
  for (int d = 0; d < DUNGEON_COUNT; d++) {
    if (!game_dungeon_unlocked(d)) continue;
    if (g_dungeons[d].lvl_max <= game_level()) train = d;
    if (!game_dungeon_cleared(d)) {
      // 最深部の敵に2レベル差まで近づいていれば挑む
      return g_dungeons[d].lvl_max <= game_level() + 2 ? d : train;
    }
  }
  return train;
}

// 町でのひとしごと: 鑑定 → 良い物に着替え → 残りを売る → 補充
static void town_chores(void) {
  if (game_drop_exists() && game_gold() >= game_drop_fee()) game_hire_agent();
  for (int i = 0; i < BAG_SIZE; i++) {
    if (game_bag(i) && !game_item_identified(game_bag(i))) game_identify_with_gold(i);
  }
  for (int i = 0; i < BAG_SIZE; i++) {
    const Item *it = game_bag(i);
    if (it && game_item_compare(it, NULL, NULL) == CMP_BETTER) game_equip_from_bag(i);
  }
  for (int i = BAG_SIZE - 1; i >= 0; i--) {
    if (game_bag(i)) game_sell_bag(i);
  }
  // 余ったお金で噂を買う。次に行くダンジョンで出る品だけを狙う（遊ぶ人と同じ選び方）
  while (s_hunt && game_rumour_count() < RUMOUR_SLOTS) {
    int d = s_next_dungeon % DUNGEON_COUNT;
    int lo = item_tier_for_level(g_dungeons[d].lvl_min);
    int hi = item_tier_for_level(g_dungeons[d].lvl_max);
    int pick = -1;
    for (int tries = 0; tries < 200 && pick < 0; tries++) {
      int entry;
      if (rand() % 4 == 0) {
        // 4回に1回は、そのダンジョンの固有・セット装備を狙う
        int i = rand() % g_special_count;
        if (g_specials[i].dungeon != d) continue;
        entry = BASE_COUNT + i;
      } else {
        int shape = rand() % SHAPE_COUNT;
        entry = shape * TIER_COUNT + lo + (hi > lo ? rand() % (hi - lo + 1) : 0);
      }
      if (!game_codex_found(entry) && !game_rumour_has(entry)) pick = entry;
    }
    if (pick < 0) break;
    int price = game_rumour_price(pick);
    if (!price || game_gold() < price * 4) break;
    if (game_buy_rumour(pick) != RUMOUR_OK) break;
  }
  while (game_potions() < MAX_POTIONS && game_buy_potion() == BUY_OK) {}
  while (game_portals() < 2 && game_buy_portal() == BUY_OK) {}
}

int main(int argc, char **argv) {
  int steps_per_day = argc > 1 ? atoi(argv[1]) : 4000;
  s_rotate = argc > 2 && strcmp(argv[2], "rotate") == 0;
  s_hunt = argc > 3 && strcmp(argv[3], "hunt") == 0;   // 噂を買って狙い撃ちする
  memset(pctest_persist_len, 0, sizeof(pctest_persist_len));
  pctest_steps_today = 0;
  game_init();
  srand(7);

  int cleared_on[DUNGEON_COUNT] = { 0 };
  int codex_full_on = 0;
  int all_cleared_on = 0;
  int deaths = 0;
  printf("Walking %d steps a day\n\n", steps_per_day);
  printf("  day   Lv   gold  codex  cleared  event\n");

  for (int day = 1; day <= MAX_DAYS; day++) {
    s_day = day;
    int before_deaths = s_hero.deaths;
    if (game_run_mode() == RUN_NONE) game_depart(pick_dungeon());
    pctest_steps_today += steps_per_day;
    game_update();
    deaths += s_hero.deaths - before_deaths;
    if (game_run_mode() == RUN_NONE) town_chores();

    int cleared = 0;
    for (int d = 0; d < DUNGEON_COUNT; d++) {
      if (!game_dungeon_cleared(d)) continue;
      cleared++;
      if (!cleared_on[d]) {
        cleared_on[d] = day;
        printf("  %4d  %3d  %5ld  %4d/%d   %d/%d    cleared %s\n", day, game_level(),
               (long)game_gold(), game_codex_found_count(), game_codex_size(), cleared, DUNGEON_COUNT,
               g_dungeons[d].name);
      }
    }
    if (cleared == DUNGEON_COUNT && !all_cleared_on) all_cleared_on = day;
    if (!codex_full_on && game_codex_found_count() >= game_codex_size()) codex_full_on = day;

    if (day % 365 == 0 || day == 30 || day == 90 || day == 180) {
      printf("  %4d  %3d  %5ld  %4d/%d   %d/%d    (%d deaths so far)\n", day, game_level(),
             (long)game_gold(), game_codex_found_count(), game_codex_size(), cleared, DUNGEON_COUNT,
             deaths);
    }
    if (codex_full_on) break;
  }

  printf("\nAll dungeons cleared: ");
  if (all_cleared_on) printf("day %d (%.1f months)\n", all_cleared_on, all_cleared_on / 30.4);
  else printf("not within %d years\n", YEARS);
  printf("Codex complete: ");
  if (codex_full_on) printf("day %d (%.1f years)\n", codex_full_on, codex_full_on / 365.0);
  else printf("not within %d years (%d / %d found)\n", YEARS, game_codex_found_count(), game_codex_size());
  printf("Deaths: %d\n", deaths);

  // 何が残っているかの内訳
  int miss_tier[TIER_COUNT] = { 0 };
  int miss_rank[4] = { 0 };
  for (int shape = 0; shape < SHAPE_COUNT; shape++) {
    for (int t = 0; t < TIER_COUNT; t++) {
      if (game_codex_found(shape * TIER_COUNT + t)) continue;
      miss_tier[t]++;
      miss_rank[g_shapes[shape].rank & 3]++;
    }
  }
  int miss_uniq = 0, miss_set = 0, miss_boss = 0;
  for (int i = 0; i < g_special_count; i++) {
    if (game_codex_found(BASE_COUNT + i)) continue;
    if (g_specials[i].set_id) miss_set++;
    else miss_uniq++;
    if (g_specials[i].boss) miss_boss++;
  }
  printf("\nStill missing by material tier:");
  for (int t = 0; t < TIER_COUNT; t++) printf(" t%d %d", t, miss_tier[t]);
  printf("\nStill missing by how rare the shape is: common %d, %d, %d, rarest %d\n",
         miss_rank[0], miss_rank[1], miss_rank[2], miss_rank[3]);
  printf("Still missing: %d uniques (%d of them boss-only), %d set pieces\n", miss_uniq, miss_boss,
         miss_set);
  return 0;
}
