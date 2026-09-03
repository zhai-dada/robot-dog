#!/usr/bin/env bash
# 验证第八天Lifecycle Node的状态转换和资源管理。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
TEMP_DIR="$(mktemp -d)"
NODE_PID=""

cleanup() {
  if [[ -n "$NODE_PID" ]]; then
    kill -INT "$NODE_PID" 2>/dev/null || true
    wait "$NODE_PID" 2>/dev/null || true
  fi
  rm -rf "$TEMP_DIR"
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day8_lifecycle
source "$WORKSPACE_DIR/install/setup.bash"

NODE_EXECUTABLE="$WORKSPACE_DIR/install/robot_dog_day8_lifecycle/lib/robot_dog_day8_lifecycle/lifecycle_status_publisher"
CONFIG_FILE="$WORKSPACE_DIR/install/robot_dog_day8_lifecycle/share/robot_dog_day8_lifecycle/config/lifecycle_status_publisher.yaml"

"$NODE_EXECUTABLE" --ros-args --params-file "$CONFIG_FILE" >"$TEMP_DIR/node.log" 2>&1 &
NODE_PID=$!

state=""
for _ in {1..10}; do
  set +e
  state="$(ros2 lifecycle get /lifecycle_status_publisher 2>/dev/null)"
  state_status=$?
  set -e
  if [[ "$state_status" -eq 0 ]]; then
    break
  fi
  sleep 1
done

[[ "$state" == *'unconfigured [1]'* ]] || {
  printf '错误: 初始生命周期状态不是unconfigured: %s\n' "$state" >&2
  exit 1
}

ros2 lifecycle set /lifecycle_status_publisher configure
state="$(ros2 lifecycle get /lifecycle_status_publisher)"
[[ "$state" == *'inactive [2]'* ]] || {
  printf '错误: configure后状态不是inactive: %s\n' "$state" >&2
  exit 1
}

set +e
inactive_messages="$(timeout 3s ros2 topic echo /robot_dog/lifecycle_status --once 2>&1)"
inactive_status=$?
set -e
[[ "$inactive_status" -ne 0 || "$inactive_messages" != *'robot_name:'* ]] || {
  printf '错误: inactive状态下不应发布业务消息\n' >&2
  exit 1
}

ros2 lifecycle set /lifecycle_status_publisher activate
state="$(ros2 lifecycle get /lifecycle_status_publisher)"
[[ "$state" == *'active [3]'* ]] || {
  printf '错误: activate后状态不是active: %s\n' "$state" >&2
  exit 1
}

set +e
active_messages="$(timeout 10s ros2 topic echo /robot_dog/lifecycle_status --once 2>&1)"
active_status=$?
set -e
[[ "$active_messages" == *'robot_name: lifecycle_robot_dog'* ]] || {
  printf '%s\n' "$active_messages" >&2
  printf '%s\n' "$(<"$TEMP_DIR/node.log")" >&2
  printf 'Topic等待状态码: %s\n' "$active_status" >&2
  printf '错误: active状态下没有收到Lifecycle消息\n' >&2
  exit 1
}

ros2 lifecycle set /lifecycle_status_publisher deactivate
state="$(ros2 lifecycle get /lifecycle_status_publisher)"
[[ "$state" == *'inactive [2]'* ]] || {
  printf '错误: deactivate后状态不是inactive: %s\n' "$state" >&2
  exit 1
}

sleep 1
set +e
deactivated_messages="$(timeout 3s ros2 topic echo /robot_dog/lifecycle_status --once 2>&1)"
deactivated_status=$?
set -e
[[ "$deactivated_status" -ne 0 || "$deactivated_messages" != *'robot_name:'* ]] || {
  printf '错误: deactivate后仍然发布业务消息\n' >&2
  exit 1
}

ros2 lifecycle set /lifecycle_status_publisher cleanup
state="$(ros2 lifecycle get /lifecycle_status_publisher)"
[[ "$state" == *'unconfigured [1]'* ]] || {
  printf '错误: cleanup后状态不是unconfigured: %s\n' "$state" >&2
  exit 1
}

ros2 param set /lifecycle_status_publisher publish_period_ms 0 >/dev/null
set +e
invalid_configure_output="$(ros2 lifecycle set /lifecycle_status_publisher configure 2>&1)"
invalid_configure_status=$?
set -e
state="$(ros2 lifecycle get /lifecycle_status_publisher)"
[[ "$state" == *'unconfigured [1]'* ]] || {
  printf '%s\n' "$invalid_configure_output" >&2
  printf '错误: 非法配置后节点没有保持unconfigured: %s\n' "$state" >&2
  exit 1
}

printf '第八天验收通过。\n'
