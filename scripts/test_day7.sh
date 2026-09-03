#!/usr/bin/env bash
# 验证第七天的QoS兼容性、不兼容事件和Transient Local行为。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
TEMP_DIR="$(mktemp -d)"
PIDS=()

cleanup() {
  for pid in "${PIDS[@]}"; do
    kill -INT "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done
  rm -rf "$TEMP_DIR"
}

stop_processes() {
  for pid in "${PIDS[@]}"; do
    kill -INT "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done
  PIDS=()
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day7_qos
source "$WORKSPACE_DIR/install/setup.bash"

PUBLISHER="$WORKSPACE_DIR/install/robot_dog_day7_qos/lib/robot_dog_day7_qos/qos_status_publisher"
SUBSCRIBER="$WORKSPACE_DIR/install/robot_dog_day7_qos/lib/robot_dog_day7_qos/qos_status_subscriber"

"$SUBSCRIBER" --ros-args -p reliability:=best_effort \
  >"$TEMP_DIR/compatible_subscriber.log" 2>&1 &
PIDS+=("$!")
sleep 2
"$PUBLISHER" --ros-args \
  -p reliability:=reliable \
  -p publish_period_ms:=200 \
  >"$TEMP_DIR/compatible_publisher.log" 2>&1 &
PIDS+=("$!")
sleep 6

[[ "$(<"$TEMP_DIR/compatible_subscriber.log")" == *'接收QoS消息'* ]] || {
  printf '错误: reliable发布者没有向best_effort订阅者发送数据\n' >&2
  exit 1
}
stop_processes
sleep 2

"$SUBSCRIBER" --ros-args -p reliability:=reliable \
  >"$TEMP_DIR/incompatible_subscriber.log" 2>&1 &
PIDS+=("$!")
sleep 2
"$PUBLISHER" --ros-args \
  -p reliability:=best_effort \
  -p publish_period_ms:=200 \
  >"$TEMP_DIR/incompatible_publisher.log" 2>&1 &
PIDS+=("$!")
sleep 6

[[ "$(<"$TEMP_DIR/incompatible_subscriber.log")" == *'QoS不兼容'* ]] || {
  printf '错误: reliable订阅者没有报告QoS不兼容\n' >&2
  exit 1
}
[[ "$(<"$TEMP_DIR/incompatible_subscriber.log")" != *'接收QoS消息'* ]] || {
  printf '错误: 不兼容QoS组合仍然接收到了消息\n' >&2
  exit 1
}
stop_processes
sleep 2

"$PUBLISHER" --ros-args \
  -p reliability:=reliable \
  -p durability:=transient_local \
  -p publish_count:=1 \
  -p publish_period_ms:=100 \
  >"$TEMP_DIR/transient_publisher.log" 2>&1 &
PIDS+=("$!")
sleep 3
"$SUBSCRIBER" --ros-args \
  -p reliability:=reliable \
  -p durability:=transient_local \
  >"$TEMP_DIR/transient_subscriber.log" 2>&1 &
PIDS+=("$!")
sleep 6

[[ "$(<"$TEMP_DIR/transient_publisher.log")" == *'停止发布并保持节点存活'* ]] || {
  printf '错误: Transient Local发布者没有停止在单条消息状态\n' >&2
  exit 1
}
[[ "$(<"$TEMP_DIR/transient_subscriber.log")" == *'sequence=0'* ]] || {
  printf '错误: 晚启动订阅者没有收到Transient Local缓存消息\n' >&2
  exit 1
}

printf '第七天验收通过。\n'
