#pragma once
// ============================================================
// PC でゲームロジックを動かすための、最小限の pebble.h の代わり
//   game.c / steps.c を gcc でそのままコンパイルするために用意している。
//   画面まわりの API は入っていない（バランス確認とアイテムの検証用）。
// ============================================================
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY 86400

#define PBL_HEALTH 1

typedef enum { HealthMetricStepCount, HealthMetricSleepSeconds } HealthMetric;
typedef enum { HealthActivitySleep = 1 } HealthActivity;
typedef uint32_t HealthActivityMask;
typedef enum { HealthIterationDirectionPast, HealthIterationDirectionFuture } HealthIterationDirection;
typedef bool (*HealthActivityIteratorCB)(HealthActivity activity, time_t time_start, time_t time_end,
                                         void *context);
typedef uint32_t HealthServiceAccessibilityMask;
#define HealthServiceAccessibilityMaskAvailable 1

// テストから歩数を決める
extern int32_t pctest_steps_today;
extern int32_t pctest_day_total;
// 睡眠: 今日の合計と、眠りの記録1件（start == end なら記録なし）
extern int32_t pctest_sleep_today;
extern time_t pctest_sleep_start, pctest_sleep_end;

static inline HealthServiceAccessibilityMask health_service_metric_accessible(HealthMetric m,
                                                                              time_t start,
                                                                              time_t end) {
  (void)m; (void)start; (void)end;
  return HealthServiceAccessibilityMaskAvailable;
}
static inline int32_t health_service_sum_today(HealthMetric m) {
  return m == HealthMetricSleepSeconds ? pctest_sleep_today : pctest_steps_today;
}
static inline void health_service_activities_iterate(HealthActivityMask mask, time_t start, time_t end,
                                                     HealthIterationDirection dir,
                                                     HealthActivityIteratorCB cb, void *context) {
  (void)mask; (void)dir;
  if (pctest_sleep_end > pctest_sleep_start && pctest_sleep_end > start && pctest_sleep_start < end) {
    cb(HealthActivitySleep, pctest_sleep_start, pctest_sleep_end, context);
  }
}
static inline int32_t health_service_sum(HealthMetric m, time_t start, time_t end) {
  (void)m; (void)start; (void)end;
  return pctest_day_total;
}

// 保存領域（メモリ上の簡単な置き換え）
#define PCTEST_PERSIST_KEYS 256
#define PCTEST_PERSIST_SIZE 256
extern uint8_t pctest_persist[PCTEST_PERSIST_KEYS][PCTEST_PERSIST_SIZE];
extern int pctest_persist_len[PCTEST_PERSIST_KEYS];

static inline bool persist_exists(uint32_t key) {
  return key < PCTEST_PERSIST_KEYS && pctest_persist_len[key] > 0;
}
static inline int persist_write_data(uint32_t key, const void *data, size_t size) {
  if (key >= PCTEST_PERSIST_KEYS || size > PCTEST_PERSIST_SIZE) return -1;
  memcpy(pctest_persist[key], data, size);
  pctest_persist_len[key] = (int)size;
  return (int)size;
}
static inline int persist_read_data(uint32_t key, void *buf, size_t size) {
  if (!persist_exists(key)) return -1;
  size_t n = size < (size_t)pctest_persist_len[key] ? size : (size_t)pctest_persist_len[key];
  memcpy(buf, pctest_persist[key], n);
  return (int)n;
}
static inline void persist_delete(uint32_t key) {
  if (key < PCTEST_PERSIST_KEYS) pctest_persist_len[key] = 0;
}

// リソース（resources/data の raw ファイルを直接読む）
typedef const char *ResHandle;
#define RESOURCE_ID_ITEM_NAMES "../../resources/data/item_names.bin"
#define RESOURCE_ID_ITEM_ICONS "../../resources/data/item_icons.bin"

static inline ResHandle resource_get_handle(const char *id) { return id; }

static inline size_t resource_load_byte_range(ResHandle h, uint32_t offset, uint8_t *buf, size_t size) {
  FILE *f = fopen(h, "rb");
  if (!f) return 0;
  size_t n = 0;
  if (fseek(f, (long)offset, SEEK_SET) == 0) n = fread(buf, 1, size, f);
  fclose(f);
  return n;
}

static inline void vibes_double_pulse(void) {}
static inline void vibes_short_pulse(void) {}
