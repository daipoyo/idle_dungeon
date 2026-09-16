#include "steps.h"

// 何日分さかのぼって数えるか（それより前は数えない）
#define MAX_DAYS_BACK 7

static time_t start_of_day(time_t t) {
  struct tm *tm = localtime(&t);
  return t - tm->tm_hour * SECONDS_PER_HOUR - tm->tm_min * SECONDS_PER_MINUTE - tm->tm_sec;
}

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
