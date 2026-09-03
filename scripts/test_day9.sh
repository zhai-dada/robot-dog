#!/usr/bin/env bash
# 验证第九天TF广播与监听。

set -eo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
TEMP_DIR="$(mktemp -d)"
PIDS=()

cleanup() {
  for pid in "${PIDS[@]}"; do
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
  done
  rm -rf "$TEMP_DIR"
}

trap cleanup EXIT

export ROS_DOMAIN_ID=109
source /opt/ros/humble/setup.bash
cd "$WORKSPACE_DIR"
colcon build --packages-up-to robot_dog_day9_tf
source "$WORKSPACE_DIR/install/setup.bash"

BROADCASTER="$WORKSPACE_DIR/install/robot_dog_day9_tf/lib/robot_dog_day9_tf/tf_broadcaster"
LISTENER="$WORKSPACE_DIR/install/robot_dog_day9_tf/lib/robot_dog_day9_tf/tf_listener"

"$BROADCASTER" >"$TEMP_DIR/broadcaster.log" 2>&1 &
PIDS+=("$!")
"$LISTENER" >"$TEMP_DIR/listener.log" 2>&1 &
PIDS+=("$!")

listener_output=""
for _ in {1..15}; do
  listener_output="$(<"$TEMP_DIR/listener.log")"
  if [[ "$listener_output" == *'TF: x='* ]]; then
    break
  fi
  sleep 1
done

[[ "$listener_output" == *'TF: x='* ]] || {
  printf '%s\n' "$listener_output" >&2
  printf '错误: TF监听节点没有查询到变换\n' >&2
  exit 1
}

printf '第九天验收通过。\n'
