#!/usr/bin/env bash
# 验证第六天的参数声明、动态更新和非法参数拒绝。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
NODE_LOG="$(mktemp)"
NODE_PID=""

cleanup() {
  if [[ -n "$NODE_PID" ]]; then
    kill -INT "$NODE_PID" 2>/dev/null || true
    wait "$NODE_PID" 2>/dev/null || true
  fi
  rm -f "$NODE_LOG"
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day6_parameters
source "$WORKSPACE_DIR/install/setup.bash"

timeout --signal=SIGINT --kill-after=2s 60s \
  ros2 run robot_dog_day6_parameters parameter_demo \
  --ros-args \
  --params-file install/robot_dog_day6_parameters/share/robot_dog_day6_parameters/config/parameter_demo.yaml \
  >"$NODE_LOG" 2>&1 &
NODE_PID=$!

robot_name=""
for _ in {1..10}; do
  set +e
  robot_name="$(ros2 param get /parameter_demo robot_name 2>/dev/null)"
  param_status=$?
  set -e
  if [[ "$param_status" -eq 0 ]]; then
    break
  fi
  sleep 1
done

[[ "$robot_name" == *'robot_dog_parameter_demo'* ]] || {
  printf '错误: 启动参数robot_name没有生效: %s\n' "$robot_name" >&2
  printf '%s\n' "$(<"$NODE_LOG")" >&2
  exit 1
}

set_output="$(ros2 param set /parameter_demo robot_name field_dog)"
[[ "$set_output" == *'Set parameter successful'* ]] || {
  printf '错误: 合法参数更新失败: %s\n' "$set_output" >&2
  exit 1
}

updated_name="$(ros2 param get /parameter_demo robot_name)"
[[ "$updated_name" == *'field_dog'* ]] || {
  printf '错误: 动态参数没有更新: %s\n' "$updated_name" >&2
  exit 1
}

set +e
invalid_output="$(ros2 param set /parameter_demo publish_rate_hz 0.0 2>&1)"
invalid_status=$?
set -e
[[ "$invalid_output" == *'Setting parameter failed'* ]] || {
  printf '错误: 非法频率没有返回失败结果: %s\n' "$invalid_output" >&2
  exit 1
}
current_rate="$(ros2 param get /parameter_demo publish_rate_hz)"
[[ "$current_rate" == *'2.0'* ]] || {
  printf '错误: 非法频率改变了当前值: %s\n' "$current_rate" >&2
  exit 1
}

printf '第六天验收通过。\n'
