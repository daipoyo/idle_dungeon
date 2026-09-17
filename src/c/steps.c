#include "steps.h"

// 何日分さかのぼって数えるか（それより前は数えない）
#define MAX_DAYS_BACK 7

time_t steps_day_start(time_t t) {
  struct tm *tm = localtime(&t);
  return t - tm->tm_hour * SECONDS_PER_HOUR - tm->tm_min * SECONDS_PER_MINUTE - tm->tm_sec;
}

static time_t start_of_day(time_t t) { return steps_day_start(t); }

bool steps_available(void) {
#if defined(PBL_HEALTH)
  time_t now = time(NULL);
  HealthServiceAccessibilityMask mask =
      health_service_metric_accessible(HealthMetricStepCount, start_of_day(now), now);
  return (mask & HealthServiceAccessibilityMaskAvailable) != 0;
#else
  return false;
#endif
}

static int32_t today_steps(void) {
#if defined(PBL_HEALTH)
  return health_service_sum_today(HealthMetricStepCount);
#else
  return 0;
#endif
}

static int32_t day_total(time_t day) {
#if defined(PBL_HEALTH)
  return health_service_sum(HealthMetricStepCount, day, day + SECONDS_PER_DAY);
#else
  return 0;
#endif
}

void steps_snapshot(StepSnapshot *snap) {
  snap->day = (uint32_t)start_of_day(time(NULL));
  snap->steps = today_steps();
}

int32_t steps_since(StepSnapshot *snap) {
  time_t today = start_of_day(time(NULL));
  int32_t now_steps = today_steps();
  int32_t delta = 0;
  time_t day = (time_t)snap->day;
  if (day == today) {
    delta = now_steps - snap->steps;
  } else if (day < today) {
    if (today - day <= MAX_DAYS_BACK * SECONDS_PER_DAY) {
      int32_t rest = day_total(day) - snap->steps;
      if (rest > 0) delta += rest;
      for (day += SECONDS_PER_DAY; day < today; day += SECONDS_PER_DAY) {
        delta += day_total(day);
      }
    }
    delta += now_steps;
  }
  // 時計が戻った・歩数がリセットされた場合は数えない
  if (delta < 0) delta = 0;
  snap->day = (uint32_t)today;
  snap->steps = now_steps;
  return delta;
}

#if defined(PBL_HEALTH)
typedef struct {
  time_t from, to;
  int32_t seconds;
} SleepSum;

static bool add_sleep(HealthActivity activity, time_t start, time_t end, void *context) {
  SleepSum *sum = context;
  if (start < sum->from) start = sum->from;
  if (end > sum->to) end = sum->to;
  if (end > start) sum->seconds += (int32_t)(end - start);
  return true;
}
#endif

int32_t steps_last_night_sleep(void) {
#if defined(PBL_HEALTH)
  time_t now = time(NULL);
  time_t today = start_of_day(now);
  SleepSum sum = { today - 6 * SECONDS_PER_HOUR, today + 12 * SECONDS_PER_HOUR, 0 };
  if (sum.to > now) sum.to = now;
  if (sum.to > sum.from) {
    health_service_activities_iterate(HealthActivitySleep, sum.from, sum.to, HealthIterationDirectionFuture,
                                      add_sleep, &sum);
  }
  // 眠りの記録が取れない場合に備えて、今日の睡眠の合計とくらべて多い方を使う
  int32_t today_total = health_service_sum_today(HealthMetricSleepSeconds);
  return sum.seconds > today_total ? sum.seconds : today_total;
#else
  return 0;
#endif
}
