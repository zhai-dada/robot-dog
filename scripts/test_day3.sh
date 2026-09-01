#!/usr/bin/env bash
# 验证第三天的自定义Service和状态切换链路。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
SERVER_LOG="$(mktemp)"
SERVER_PID=""

cleanup() {
  if [[ -n "$SERVER_PID" ]]; then
    kill -INT "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  rm -f "$SERVER_LOG"
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
if [[ -z "${ROS_DISTRO:-}" ]]; then
  printf '错误: ROS2环境未设置\n' >&2
  exit 1
fi

cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day3_services
source "$WORKSPACE_DIR/install/setup.bash"

interface_output="$(ros2 interface show robot_dog_interfaces/srv/SetRobotState)"
for field in \
  'string target_state' \
  'bool success' \
  'string message' \
  'string current_state'; do
  [[ "$interface_output" == *"$field"* ]] || {
    printf '错误: Service接口缺少字段: %s\n' "$field" >&2
    exit 1
  }
done

timeout --signal=SIGINT --kill-after=2s 15s \
  ros2 run robot_dog_day3_services state_server \
  --ros-args \
  --params-file install/robot_dog_day3_services/share/robot_dog_day3_services/config/state_server.yaml \
  >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

service_type=""
for _ in {1..10}; do
  set +e
  service_type="$(ros2 service type /robot_dog/set_state 2>/dev/null)"
  service_type_status=$?
  set -e
  if [[ "$service_type_status" -eq 0 ]]; then
    break
  fi
  sleep 1
done

[[ "$service_type" == 'robot_dog_interfaces/srv/SetRobotState' ]] || {
  printf '错误: 服务没有按预期注册\n' >&2
  printf '%s\n' "$(<"$SERVER_LOG")" >&2
  exit 1
}

set +e
valid_output="$(ros2 run robot_dog_day3_services state_client \
  --ros-args -p target_state:=TROT 2>&1)"
valid_status=$?
invalid_output="$(ros2 run robot_dog_day3_services state_client \
  --ros-args -p target_state:=FLYING 2>&1)"
invalid_status=$?
set -e

[[ "$valid_status" -eq 0 ]] || {
  printf '%s\n' "$valid_output" >&2
  printf '错误: 合法状态请求失败，状态码=%s\n' "$valid_status" >&2
  exit 1
}
[[ "$valid_output" == *'current_state=TROT'* ]] || {
  printf '%s\n' "$valid_output" >&2
  printf '错误: 合法请求没有返回TROT\n' >&2
  exit 1
}

[[ "$invalid_status" -eq 2 ]] || {
  printf '%s\n' "$invalid_output" >&2
  printf '错误: 非法状态没有返回业务拒绝码2\n' >&2
  exit 1
}
[[ "$invalid_output" == *'current_state=TROT'* ]] || {
  printf '%s\n' "$invalid_output" >&2
  printf '错误: 非法请求改变了当前状态\n' >&2
  exit 1
}

printf '第三天验收通过。\n'
