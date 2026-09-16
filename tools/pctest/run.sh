#!/usr/bin/env bash
# ゲームロジックを PC 上で動かして確認する（時計のエミュレーターより速い）
set -euo pipefail
cd "$(dirname "$0")"
gcc -std=c11 -O1 -g -Wall -Wextra -Wno-unused-parameter -I. -o /tmp/idle_pctest test_game.c
/tmp/idle_pctest
