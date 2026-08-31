#!/usr/bin/env bash
# 验证第二天的消息接口和Topic发布/订阅链路。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
TEMP_DIR="$(mktemp -d)"
SUBSCRIBER_LOG="$TEMP_DIR/status_subscriber.log"

cleanup() {
  if [[ -n "${SUBSCRIBER_PID:-}" ]]; then
    kill -INT "$SUBSCRIBER_PID" 2>/dev/null || true
    wait "$SUBSCRIBER_PID" 2>/dev/null || true
  fi
  rm -rf "$TEMP_DIR"
}

trap cleanup EXIT

source /opt/ros/humble/setup.bash
if [[ -z "${ROS_DISTRO:-}" ]]; then
  printf '错误: ROS2环境未设置\n' >&2
  exit 1
fi

cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day2_topics
source "$WORKSPACE_DIR/install/setup.bash"

interface_output="$(ros2 interface show robot_dog_interfaces/msg/RobotStatus)"
for field in \
  'string robot_name' \
  'string state' \
  'uint64 sequence' \
  'float32 battery_percentage'; do
  [[ "$interface_output" == *"$field"* ]] || {
    printf '错误: 消息接口缺少字段: %s\n' "$field" >&2
    exit 1
  }
done

timeout --signal=SIGINT --kill-after=2s 10s \
  ros2 run robot_dog_day2_topics status_subscriber >"$SUBSCRIBER_LOG" 2>&1 &
SUBSCRIBER_PID=$!
sleep 2

set +e
publisher_output="$(timeout --signal=SIGINT --kill-after=2s 4s \
  ros2 run robot_dog_day2_topics status_publisher \
  --ros-args \
  -p robot_name:=test_dog \
  -p state:=TROT \
  -p battery_percentage:=87.5 \
  -p publish_period_ms:=200 2>&1)"
publisher_status=$?
set -e

if [[ "$publisher_status" -ne 0 && "$publisher_status" -ne 124 && "$publisher_status" -ne 130 ]]; then
  printf '%s\n' "$publisher_output" >&2
  printf '错误: Publisher异常退出，状态码=%s\n' "$publisher_status" >&2
  exit 1
fi

sleep 1
[[ "$publisher_output" == *'robot_name=test_dog'* ]] || {
  printf '%s\n' "$publisher_output" >&2
  printf '错误: Publisher参数没有生效\n' >&2
  exit 1
}

[[ -s "$SUBSCRIBER_LOG" ]] || {
  printf '错误: Subscriber没有产生日志\n' >&2
  exit 1
}

[[ "$(<"$SUBSCRIBER_LOG")" == *'robot_name=test_dog'* ]] || {
  printf '错误: Subscriber没有收到robot_name=test_dog\n' >&2
  exit 1
}
[[ "$(<"$SUBSCRIBER_LOG")" == *'state=TROT'* ]] || {
  printf '错误: Subscriber没有收到state=TROT\n' >&2
  exit 1
}
[[ "$(<"$SUBSCRIBER_LOG")" == *'battery=87.5%'* ]] || {
  printf '错误: Subscriber没有收到battery=87.5%%\n' >&2
  exit 1
}

topic_type="$(ros2 topic type /robot_dog/status)"
[[ "$topic_type" == 'robot_dog_interfaces/msg/RobotStatus' ]] || {
  printf '错误: Topic类型错误: %s\n' "$topic_type" >&2
  exit 1
}

set +e
timeout --signal=SIGINT --kill-after=2s 5s \
  ros2 run robot_dog_day2_topics status_publisher \
  --ros-args -p publish_period_ms:=0 >/dev/null 2>&1
invalid_period_status=$?
timeout --signal=SIGINT --kill-after=2s 5s \
  ros2 run robot_dog_day2_topics status_publisher \
  --ros-args -p battery_percentage:=101.0 >/dev/null 2>&1
invalid_battery_status=$?
set -e

[[ "$invalid_period_status" -ne 0 ]] || {
  printf '错误: 非法发布周期没有被拒绝\n' >&2
  exit 1
}
[[ "$invalid_battery_status" -ne 0 ]] || {
  printf '错误: 非法电量没有被拒绝\n' >&2
  exit 1
}

printf '第二天验收通过。\n'
