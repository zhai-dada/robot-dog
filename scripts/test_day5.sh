#!/usr/bin/env bash
# 验证第五天Python Launch批量启动。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"

source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_bringup
source "$WORKSPACE_DIR/install/setup.bash"

arguments="$(ros2 launch robot_bringup robot_dog_demo.launch.py --show-args)"
[[ "$arguments" == *'robot_name'* ]] || {
  printf '错误: Launch没有声明robot_name参数\n' >&2
  exit 1
}
[[ "$arguments" == *'initial_state'* ]] || {
  printf '错误: Launch没有声明initial_state参数\n' >&2
  exit 1
}

set +e
launch_output="$(timeout --signal=SIGINT --kill-after=3s 12s \
  ros2 launch robot_bringup robot_dog_demo.launch.py \
  robot_name:=launch_test \
  initial_state:=SIT 2>&1)"
launch_status=$?
set -e

if [[ "$launch_status" -ne 0 && "$launch_status" -ne 124 && "$launch_status" -ne 130 ]]; then
  printf '%s\n' "$launch_output" >&2
  printf '错误: Launch异常退出，状态码=%s\n' "$launch_status" >&2
  exit 1
fi

for expected in \
  'status_publisher' \
  'status_subscriber' \
  'state_server' \
  'gait_action_server' \
  'parameter_demo' \
  'launch_test'; do
  [[ "$launch_output" == *"$expected"* ]] || {
    printf '%s\n' "$launch_output" >&2
    printf '错误: Launch输出缺少节点或参数: %s\n' "$expected" >&2
    exit 1
  }
done

printf '第五天验收通过。\n'
