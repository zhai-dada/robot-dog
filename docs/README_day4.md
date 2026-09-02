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

### 为什么不能只使用普通的bool

如果先检查标志、再单独设置标志，两个并发Goal可能同时读到`false`，随后都开始执行。这个过程不是原子的。

`std::atomic<bool>`只能保证单次读写安全。这里使用CAS把“检查”和“占用”合成一个原子操作：

```cpp
bool expected = false;
if (!is_executing_.compare_exchange_strong(expected, true))
{
  return rclcpp_action::GoalResponse::REJECT;
}
```

只有一个Goal能把标志从`false`改为`true`，其他并发Goal会在`handleGoal()`阶段被拒绝。

### 为什么要在handleGoal中拒绝

Action处理分成两个阶段：

```text
handleGoal      -> 判断是否接受Goal
handleAccepted  -> Goal已经接受，开始执行
```

如果在`handleAccepted()`里发现忙碌后直接返回，Goal已经被ROS2标记为接受，但服务端没有发送Result，客户端就可能永久等待。因此互斥判断必须放在`handleGoal()`。

### 为什么需要RAII释放执行权

执行权必须在正常完成、用户取消和执行异常三条路径释放。服务端使用局部`ExecutionFlagGuard`，离开`execute()`作用域时由析构函数自动恢复`is_executing_`。

这相当于Linux驱动中用作用域对象管理锁、文件描述符或映射资源，减少遗漏释放的风险。

### 为什么使用单独执行线程

步态执行可能持续数秒，不能阻塞ROS2通信线程，否则取消请求可能无法及时处理。示例使用一个专用执行线程，ROS2线程继续处理Goal和Cancel。

这个线程只模拟高层任务执行，不是实时控制线程。真实系统中，Action线程应向固定周期的步态控制器发送目标，控制器再生成关节命令。

### 取消的实际过程

Ctrl+C不是直接杀死服务端线程，而是：

```text
客户端捕获SIGINT
    -> 发送Cancel请求
    -> 服务端接受Cancel
    -> 执行线程在安全检查点退出
    -> 返回Canceled和completed_steps
    -> 释放执行权
```

服务端每10毫秒检查一次取消状态，因此取消响应延迟通常受通信延迟和当前检查周期影响。

### 三种单任务管理方案

1. `std::atomic<bool>`加CAS

   适合只有“空闲/忙碌”两个状态，代码最少。本示例采用该方案。

2. `std::mutex`加活动Goal句柄

   适合还需要记录当前Goal、步态名称、取消原因或任务拥有者。互斥锁保护一组共享状态，比多个独立原子变量更容易保持一致。

3. 任务队列或抢占策略

   适合工业任务管理。新Goal可以排队、拒绝，或者取消旧Goal后抢占执行。这个方案需要明确优先级和安全停机策略，不适合作为第一版直接实现。

对于当前练习，CAS不是低级方案，而是最小正确方案。等状态字段增加后，再升级为`mutex + ActiveGoal`结构。

### 当前示例的边界

当前实现保证客户端Ctrl+C会先请求取消。服务端进程自身在执行Goal时直接退出，涉及Action终态发布与节点生命周期顺序，不能只靠一个原子变量解决。后续学习Lifecycle Node时，再实现“停止接收新Goal、取消活动Goal、等待控制器停稳、最后关闭节点”的完整流程。

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
   - 验证成功任务、非法Goal、并发Goal和取消处理。

### 设计原因

- `gait_name`限定为`CRAWL`、`WALK`和`TROT`，避免未知步态进入执行层。
- `step_count`使任务长度可控，方便仿真和测试。
- `step_period_ms`模拟控制周期，但不代表最终硬实时周期。
- 客户端用进程返回码区分成功、服务不可用和Goal被拒绝。
- 客户端收到Ctrl+C时返回130，并向服务端发送Cancel请求。

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

### 逐步观察取消

启动一个较长任务：

```bash
ros2 run robot_dog_day4_actions gait_action_client \
  --ros-args -p gait_name:=TROT -p step_count:=20 -p step_period_ms:=200
```

在看到几次Feedback后按`Ctrl+C`。预期结果：

1. 客户端输出正在取消目标。
2. 客户端返回码为130。
3. 服务端收到取消请求。
4. 服务端返回已完成步数，而不是完整成功。
5. 服务端之后仍可以接受新的Goal。

### 观察并发Goal

在第一个任务执行期间，另一个终端发送：

```bash
ros2 run robot_dog_day4_actions gait_action_client \
  --ros-args -p gait_name:=CRAWL -p step_count:=2
```

第二个Goal应在`handleGoal()`中被拒绝，客户端返回码为2。

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

测试覆盖Action接口生成、Action Server注册、合法Goal执行、Feedback输出、非法Goal拒绝、并发Goal拒绝和Ctrl+C取消。

## 实践任务

1. 用`std::mutex`和活动Goal句柄改写单任务互斥逻辑，与CAS方案比较。
2. 增加“同一时间只允许一个步态任务”的统计信息。
3. 将模拟等待替换为后续四足控制器接口调用。
4. 设计固定周期控制线程，解释为什么步态控制不能依赖Action线程的定时精度。
5. 测量取消请求到服务端返回Canceled之间的延迟。

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
- 并发Goal在接受阶段被拒绝。
- Ctrl+C取消后，服务端释放执行权并可以再次接受任务。

## 下一步

第五天学习Python Launch，用一个启动文件同时启动状态、服务、Action和参数节点。
