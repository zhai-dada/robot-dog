#!/usr/bin/env bash
# 验证第一天的ROS2节点、倒计时行为和静态检查。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"

source /opt/ros/humble/setup.bash

if [[ -z "${ROS_DISTRO:-}" ]]; then
  printf '错误: ROS2环境未设置\n' >&2
  exit 1
fi

printf 'ROS2版本: %s\n' "$ROS_DISTRO"

required_paths=(
  "$WORKSPACE_DIR/src/robot_dog_basics"
  "$WORKSPACE_DIR/src/robot_dog_basics_cpp"
  "$WORKSPACE_DIR/src/robot_dog_day1_exercises"
  "$WORKSPACE_DIR/docs/README_day1.md"
)

for path in "${required_paths[@]}"; do
  [[ -e "$path" ]] || {
    printf '错误: 缺少路径 %s\n' "$path" >&2
    exit 1
  }
done

colcon build \
  --packages-select \
  robot_dog_basics \
  robot_dog_basics_cpp \
  robot_dog_day1_exercises
source "$WORKSPACE_DIR/install/setup.bash"
set -u

run_for_a_moment() {
  local package_name="$1"
  local executable_name="$2"
  local expected_output="$3"
  local output
  local status

  set +e
  output="$(timeout --signal=SIGINT --kill-after=2s 4s \
    ros2 run "$package_name" "$executable_name" 2>&1)"
  status=$?
  set -e

  if [[ "$status" -ne 0 && "$status" -ne 124 && "$status" -ne 130 ]]; then
    printf '%s\n' "$output" >&2
    printf '错误: %s/%s异常退出，状态码=%s\n' \
      "$package_name" "$executable_name" "$status" >&2
    exit 1
  fi

  [[ "$output" == *"$expected_output"* ]] || {
    printf '%s\n' "$output" >&2
    printf '错误: %s/%s没有输出预期内容\n' \
      "$package_name" "$executable_name" >&2
    exit 1
  }
}

run_for_a_moment robot_dog_basics hello_world_node '机器人狗系统工程'
run_for_a_moment robot_dog_basics_cpp hello_world_node '机器人狗系统工程'

set +e
countdown_output="$(timeout --signal=SIGINT --kill-after=2s 55s \
  ros2 run robot_dog_day1_exercises counter_node 2>&1)"
countdown_status=$?
set -e

if [[ "$countdown_status" -ne 0 && "$countdown_status" -ne 124 && "$countdown_status" -ne 130 ]]; then
  printf '%s\n' "$countdown_output" >&2
  printf '错误: counter_node异常退出，状态码=%s\n' "$countdown_status" >&2
  exit 1
fi

[[ "$countdown_output" == *'计数: 100'* ]] || {
  printf '错误: counter_node没有从100开始\n' >&2
  exit 1
}
[[ "$countdown_output" == *'计数: 0'* ]] || {
  printf '错误: counter_node没有运行到0\n' >&2
  exit 1
}

ament_flake8 "$WORKSPACE_DIR/src/robot_dog_basics/robot_dog_basics"
ament_pep257 "$WORKSPACE_DIR/src/robot_dog_basics/robot_dog_basics"
colcon test \
  --packages-select \
  robot_dog_basics_cpp \
  robot_dog_day1_exercises

printf '第一天验收通过。\n'
