#!/usr/bin/env bash
# 長く遊んだときの進み方を調べる（1日の歩数を引数で渡せる。既定 4000）
set -euo pipefail
cd "$(dirname "$0")"
gcc -std=c11 -O2 -w -I. -o /tmp/idle_sim sim_progress.c
/tmp/idle_sim "$@"
