#!/usr/bin/env bash
# 第一阶段综合验收。各天测试必须串行执行，避免竞争同一个install目录。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

for test_script in \
  test_day1.sh \
  test_day2.sh \
  test_day3.sh \
  test_day4.sh \
  test_day5.sh \
  test_day6.sh \
  test_day7.sh \
  test_day8.sh \
  test_day9.sh; do
  printf '运行综合验收: %s\n' "$test_script"
  "$SCRIPT_DIR/$test_script"
done

printf '第一阶段综合验收通过。\n'
