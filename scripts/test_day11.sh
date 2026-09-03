#!/usr/bin/env bash
# 验证第十一天工具库、控制节点和模拟驱动链路。

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

trap cleanup EXIT

export ROS_DOMAIN_ID=111
source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_control robot_drivers
source "$WORKSPACE_DIR/install/setup.bash"

DRIVER="$WORKSPACE_DIR/install/robot_drivers/lib/robot_drivers/fake_motor_driver"
CONTROL="$WORKSPACE_DIR/install/robot_control/lib/robot_control/joint_command_publisher"

"$DRIVER" >"$TEMP_DIR/driver.log" 2>&1 &
PIDS+=("$!")
"$CONTROL" >"$TEMP_DIR/control.log" 2>&1 &
PIDS+=("$!")
sleep 5

[[ "$(<"$TEMP_DIR/control.log")" == *'关节控制节点已启动'* ]] || {
  printf '错误: 控制节点没有启动\n' >&2
  exit 1
}
[[ "$(<"$TEMP_DIR/driver.log")" == *'模拟电机接收目标'* ]] || {
  printf '%s\n' "$(<"$TEMP_DIR/driver.log")" >&2
  printf '错误: 模拟驱动没有收到控制目标\n' >&2
  exit 1
}

ros2 topic pub --once /robot_dog/joint_commands \
  std_msgs/msg/Float64MultiArray \
  "{data: [2.0, -2.0, 0.5]}" >/dev/null
sleep 2
[[ "$(<"$TEMP_DIR/driver.log")" == *'[1.570, -1.570, 0.500]'* ]] || {
  printf '%s\n' "$(<"$TEMP_DIR/driver.log")" >&2
  printf '错误: 模拟驱动没有正确限制关节目标\n' >&2
  exit 1
}

printf '第十一天验收通过。\n'
