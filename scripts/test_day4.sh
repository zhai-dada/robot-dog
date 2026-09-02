#!/usr/bin/env bash
# 验证第四天的自定义Action和步态执行链路。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
SERVER_LOG="$(mktemp)"
LONG_CLIENT_LOG="$(mktemp)"
SERVER_PID=""
LONG_CLIENT_PID=""

cleanup() {
  if [[ -n "$SERVER_PID" ]]; then
    kill -INT "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  if [[ -n "$LONG_CLIENT_PID" ]]; then
    kill -INT "$LONG_CLIENT_PID" 2>/dev/null || true
    wait "$LONG_CLIENT_PID" 2>/dev/null || true
  fi
  rm -f "$SERVER_LOG" "$LONG_CLIENT_LOG"
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day4_actions
source "$WORKSPACE_DIR/install/setup.bash"
SERVER_EXECUTABLE="$WORKSPACE_DIR/install/robot_dog_day4_actions/lib/robot_dog_day4_actions/gait_action_server"
CLIENT_EXECUTABLE="$WORKSPACE_DIR/install/robot_dog_day4_actions/lib/robot_dog_day4_actions/gait_action_client"

interface_output="$(ros2 interface show robot_dog_interfaces/action/ExecuteGait)"
for field in \
  'string gait_name' \
  'uint32 step_count' \
  'uint32 step_period_ms' \
  'bool success' \
  'uint32 completed_steps' \
  'uint32 current_step' \
  'uint32 total_steps' \
  'string state'; do
  [[ "$interface_output" == *"$field"* ]] || {
    printf '错误: Action接口缺少字段: %s\n' "$field" >&2
    exit 1
  }
done

timeout --signal=SIGINT --kill-after=2s 80s \
  "$SERVER_EXECUTABLE" >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

action_list=""
for _ in {1..10}; do
  set +e
  action_list="$(ros2 action list --show-types 2>/dev/null)"
  action_list_status=$?
  set -e
  if [[ "$action_list_status" -eq 0 && "$action_list" == *'/robot_dog/execute_gait [robot_dog_interfaces/action/ExecuteGait]'* ]]; then
    break
  fi
  sleep 1
done

[[ "$action_list" == *'/robot_dog/execute_gait [robot_dog_interfaces/action/ExecuteGait]'* ]] || {
  printf '错误: Action没有按预期注册\n' >&2
  printf '%s\n' "$(<"$SERVER_LOG")" >&2
  exit 1
}

"$CLIENT_EXECUTABLE" \
  --ros-args \
  -p gait_name:=TROT \
  -p step_count:=600 \
  -p step_period_ms:=100 \
  -p timeout_ms:=5000 >"$LONG_CLIENT_LOG" 2>&1 &
LONG_CLIENT_PID=$!
sleep 1

set +e
busy_output="$("$CLIENT_EXECUTABLE" \
  --ros-args \
  -p gait_name:=CRAWL \
  -p step_count:=2 \
  -p step_period_ms:=100 \
  -p timeout_ms:=3000 2>&1)"
busy_status=$?
kill -INT "$LONG_CLIENT_PID" 2>/dev/null || true
wait "$LONG_CLIENT_PID"
cancel_status=$?
set -e
LONG_CLIENT_PID=""

[[ "$busy_status" -eq 2 ]] || {
  printf '%s\n' "$busy_output" >&2
  printf '错误: 执行中第二个Goal没有被拒绝\n' >&2
  exit 1
}
[[ "$cancel_status" -eq 130 ]] || {
  printf '%s\n' "$(<"$LONG_CLIENT_LOG")" >&2
  printf '错误: Ctrl+C后的客户端返回码不是130: %s\n' "$cancel_status" >&2
  exit 1
}
[[ "$(<"$SERVER_LOG")" == *'步态执行已取消'* ]] || {
  printf '%s\n' "$(<"$SERVER_LOG")" >&2
  printf '错误: 服务端没有完成取消处理\n' >&2
  exit 1
}

set +e
valid_output="$("$CLIENT_EXECUTABLE" \
  --ros-args \
  -p gait_name:=TROT \
  -p step_count:=3 \
  -p step_period_ms:=100 \
  -p timeout_ms:=3000 2>&1)"
valid_status=$?
invalid_output="$("$CLIENT_EXECUTABLE" \
  --ros-args \
  -p gait_name:=FLY \
  -p step_count:=3 \
  -p step_period_ms:=100 \
  -p timeout_ms:=3000 2>&1)"
invalid_status=$?
set -e

[[ "$valid_status" -eq 0 ]] || {
  printf '%s\n' "$valid_output" >&2
  printf '错误: 合法Action目标执行失败，状态码=%s\n' "$valid_status" >&2
  exit 1
}
[[ "$valid_output" == *'completed_steps=3'* ]] || {
  printf '%s\n' "$valid_output" >&2
  printf '错误: Action结果没有报告3个完成步数\n' >&2
  exit 1
}

[[ "$invalid_status" -eq 2 ]] || {
  printf '%s\n' "$invalid_output" >&2
  printf '错误: 非法Action目标没有返回业务拒绝码2\n' >&2
  exit 1
}

printf '第四天验收通过。\n'
