# 第四天：ROS2 Action通信

## 今日目标

1. 理解Action与Topic、Service的区别。
2. 使用C++实现Action Server和Action Client。
3. 为步态执行任务提供Goal、Feedback和Result。
4. 理解长时间任务、取消和执行线程。

## 背景知识

第三天的Service适合快速完成的一次性请求，例如切换机器人状态。步态执行可能持续数秒甚至更久，客户端需要知道执行进度，也可能在执行中取消任务。因此它适合Action。

```text
gait_action_client  -- Goal -->  gait_action_server
gait_action_client  <-- Feedback -- gait_action_server
gait_action_client  <-- Result -- gait_action_server
gait_action_client  -- Cancel --> gait_action_server
```

## 原理

Action接口由三部分组成：

```text
Goal:     gait_name, step_count, step_period_ms
Feedback: current_step, total_steps, state
Result:   success, message, completed_steps
```

服务端在独立执行线程中处理Goal，周期性发布Feedback，任务结束后发送Result。取消请求不会伪造成功，而是返回Canceled状态和已经完成的步数。

### 与Linux嵌入式联系

| ROS2 Action | Linux/嵌入式类比 |
| --- | --- |
| Goal | ioctl或RPC中的长任务命令 |
| Feedback | 驱动事件、进度回调或异步状态上报 |
| Result | 工作队列完成通知和最终错误码 |
| Cancel | 设备停止命令或线程取消请求 |

在真实机器人中，Action层不应直接阻塞控制线程。它负责任务编排，具体关节控制应交给控制器的周期线程。

## 工程实现

### 文件路径

1. `src/robot_dog_interfaces/action/ExecuteGait.action`
   - 定义步态任务接口。
2. `src/robot_dog_day4_actions/src/gait_action_server.cpp`
   - 校验Goal，模拟步态执行，发布Feedback和Result。
3. `src/robot_dog_day4_actions/src/gait_action_client.cpp`
   - 发送Goal，等待Result并打印Feedback。
4. `scripts/test_day4.sh`
   - 验证成功任务、非法Goal和Action类型。

### 设计原因

- `gait_name`限定为`CRAWL`、`WALK`和`TROT`，避免未知步态进入执行层。
- `step_count`使任务长度可控，方便仿真和测试。
- `step_period_ms`模拟控制周期，但不代表最终硬实时周期。
- 客户端用进程返回码区分成功、服务不可用和Goal被拒绝。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day4_actions
source install/setup.bash
```

启动服务端：

```bash
ros2 run robot_dog_day4_actions gait_action_server
```

另一个终端启动客户端：

```bash
ros2 run robot_dog_day4_actions gait_action_client \
  --ros-args \
  -p gait_name:=TROT \
  -p step_count:=5 \
  -p step_period_ms:=300
```

也可以使用命令行工具观察Action：

```bash
ros2 action list
ros2 action info /robot_dog/execute_gait
ros2 action send_goal \
  /robot_dog/execute_gait \
  robot_dog_interfaces/action/ExecuteGait \
  "{gait_name: CRAWL, step_count: 3, step_period_ms: 200}" \
  --feedback
```

## 调试方法

```bash
ros2 node list
ros2 node info /gait_action_server
ros2 action list
ros2 action info /robot_dog/execute_gait
ros2 interface show robot_dog_interfaces/action/ExecuteGait
ros2 doctor
```

常见问题：

- Action列表为空：服务端未启动或工作空间未source。
- Goal被拒绝：检查步态名称、步数和周期是否为允许值。
- 没有Feedback：检查服务端是否真正进入执行线程。
- 客户端超时：检查Action服务端是否在线，以及超时时间是否覆盖任务长度。

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day4.sh
```

测试覆盖Action接口生成、Action Server注册、合法Goal执行、Feedback输出和非法Goal拒绝。

## 实践任务

1. 为客户端增加取消参数，在执行中调用`async_cancel_goal`。
2. 服务端增加“同一时间只允许一个步态任务”的限制。
3. 将执行线程中的睡眠替换为后续四足控制器接口调用。
4. 解释为什么步态控制不能只依赖Action回调线程的定时精度。

## 研发流程记录

### 输入

- 第三天的状态切换Service。
- “执行一段步态并可观察进度”的需求。

### 输出

- 自定义Action接口。
- C++服务端和客户端。
- Goal校验、Feedback、Result和Cancel处理。

### 验证方式

- `ros2 action info`检查服务发现。
- 客户端完成`TROT`任务。
- 非法Goal返回拒绝。
- `scripts/test_day4.sh`通过。

## 下一步

第五天学习Python Launch，用一个启动文件同时启动状态、服务、Action和参数节点。
