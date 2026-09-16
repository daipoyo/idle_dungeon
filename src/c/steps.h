#pragma once
#include <pebble.h>

// ============================================================
// 歩数の取得
//   health_service_sum() は途中の時間帯を指定すると「1日の合計を時間で按分した値」
//   になり正確ではない。そこで「今日の歩数」を確認のたびに記録しておき、
//   前回との差を足していく。日をまたいだ場合は、過ぎた日の1日分の合計を使う。
// ============================================================

typedef struct {
  uint32_t day;      // 記録した日の 0時（UNIX時間）
  int32_t steps;     // その日の、記録した時点での歩数
} StepSnapshot;

bool steps_available(void);
// 今の歩数を記録する（出発時に呼ぶ）
void steps_snapshot(StepSnapshot *snap);
// 前回の記録からの歩数を返し、記録を更新する
int32_t steps_since(StepSnapshot *snap);
