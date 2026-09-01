# 第三天：ROS2 Service通信

## 今日目标

1. 理解Service的请求/响应模型。
2. 使用C++实现Service Server和Client。
3. 创建并使用自定义Service接口。
4. 为机器人实现“切换状态”服务。
5. 区分Topic、Service和后续Action的适用场景。

## 背景知识

第二天的Topic适合连续数据流，例如关节状态、IMU和激光雷达。第三天增加一个有明确结果的命令：请求机器人切换状态。

```text
state_client  -- SetRobotState request -->  /robot_dog/set_state
state_client  <-- SetRobotState response --  state_server
```

状态切换是一个典型的Service场景：客户端发起一次请求，服务端执行校验并返回结果。它不是持续广播，也不是需要持续反馈的长时间任务。

## 原理

### Service组成

Service由两部分组成：

```text
请求（Request）
string target_state
---
响应（Response）
bool success
string message
string current_state
```

- **Server**注册服务名并等待请求。
- **Client**通过服务名查找Server并发送Request。
- Server回调处理Request，填充Response。
- Client等待Future完成，再读取Response。

Service的发现和传输仍由ROS2底层DDS负责，但业务语义是一次请求对应一次响应。

### 与Linux嵌入式联系

| ROS2概念 | Linux/嵌入式类比 | 工程含义 |
| --- | --- | --- |
| Service Server | RPC服务端、ioctl处理路径 | 接收命令并返回结果 |
| Service Client | RPC客户端、ioctl调用者 | 同步等待一次操作结果 |
| Request | ioctl参数或RPC参数结构体 | 描述操作意图 |
| Response | 返回值和错误码 | 告诉调用者是否执行成功 |
| `wait_for_service` | 等待设备节点或服务就绪 | 避免启动顺序造成误判 |

与设备驱动的`ioctl`类似，状态切换Service负责命令入口，但真正的电机执行还应由控制器和硬件接口完成。本示例只实现状态机层，不直接控制电机。

## Topic、Service和Action的选择

| 通信方式 | 适合场景 | robot-dog例子 |
| --- | --- | --- |
| Topic | 连续、异步、可多对多的数据流 | `/robot_dog/status`、IMU、LaserScan |
| Service | 快速完成的一次请求/响应 | `/robot_dog/set_state`、清零里程计 |
| Action | 可能持续较久，需要反馈和取消 | 导航到目标点、执行一段步态 |

不要用Service承载持续状态流，也不要用Topic伪造必须确认结果的命令。

## 接口设计

### 文件路径

`src/robot_dog_interfaces/srv/SetRobotState.srv`

### 接口内容

```text
string target_state
---
bool success
string message
string current_state
```

### 设计原因

- `target_state`明确描述客户端意图。
- `success`是机器可判断的结果，不要求解析日志文本。
- `message`用于诊断和人类阅读。
- `current_state`让客户端在成功或失败时都能知道服务端状态。

服务端只允许以下状态：

```text
STANDING, SIT, CRAWL, TROT
```

非法状态不会修改服务端状态，并返回`success=false`。这是控制命令进入执行层之前的第一道约束。

## 工程实现

### 文件作用

1. `src/robot_dog_interfaces/srv/SetRobotState.srv`
   - 定义共享Service接口。
2. `src/robot_dog_interfaces/CMakeLists.txt`
   - 使用`rosidl_generate_interfaces`生成C++和Python类型支持。
3. `src/robot_dog_day3_services/src/state_server.cpp`
   - 注册`/robot_dog/set_state`。
   - 校验目标状态并维护当前状态。
4. `src/robot_dog_day3_services/src/state_client.cpp`
   - 从参数读取目标状态。
   - 等待服务、异步发送请求并等待Future。
5. `src/robot_dog_day3_services/config/state_server.yaml`
   - 配置服务端初始状态。
6. `scripts/test_day3.sh`
   - 自动验证接口、服务发现、合法请求和非法请求。

### 服务端关键代码

```cpp
service_ = this->create_service<robot_dog_interfaces::srv::SetRobotState>(
  "robot_dog/set_state",
  std::bind(
    &StateServer::handleRequest,
    this,
    std::placeholders::_1,
    std::placeholders::_2));
```

回调先校验状态，再决定是否修改`current_state_`：

```cpp
if (!isValidState(request->target_state))
{
  response->success = false;
  response->message = "unsupported target state: " + request->target_state;
  response->current_state = current_state_;
  return;
}

current_state_ = request->target_state;
response->success = true;
response->current_state = current_state_;
```

### 客户端关键代码

```cpp
if (!client_->wait_for_service(std::chrono::milliseconds(timeout_ms_)))
{
  return 1;
}

auto future = client_->async_send_request(request);
const auto result = rclcpp::spin_until_future_complete(
  this->get_node_base_interface(), future,
  std::chrono::milliseconds(timeout_ms_));
```

客户端通过返回码区分三类结果：

- `0`：请求成功。
- `1`：服务不可用、超时或节点异常。
- `2`：服务在线，但业务拒绝请求。

这比只打印日志更适合自动化测试和上层任务管理器。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day3_services
source install/setup.bash
```

### 查看Service接口

```bash
ros2 interface show robot_dog_interfaces/srv/SetRobotState
```

### 启动服务端

终端一：

```bash
ros2 run robot_dog_day3_services state_server \
  --ros-args \
  --params-file install/robot_dog_day3_services/share/robot_dog_day3_services/config/state_server.yaml
```

### 使用C++客户端

终端二：

```bash
ros2 run robot_dog_day3_services state_client \
  --ros-args -p target_state:=TROT
```

成功时返回类似结果：

```text
服务成功: state changed from STANDING to TROT, current_state=TROT
```

发送非法状态：

```bash
ros2 run robot_dog_day3_services state_client \
  --ros-args -p target_state:=FLYING
```

服务端会拒绝请求，当前状态不会改变。

### 使用ROS2命令行客户端

```bash
ros2 service call \
  /robot_dog/set_state \
  robot_dog_interfaces/srv/SetRobotState \
  "{target_state: CRAWL}"
```

## 调试方法

按照项目规定的顺序检查：

### 1. 节点

```bash
ros2 node list
ros2 node info /state_server
ros2 node info /state_client
```

### 2. Service

```bash
ros2 service list
ros2 service type /robot_dog/set_state
ros2 service info /robot_dog/set_state
```

### 3. 数据

```bash
ros2 service call \
  /robot_dog/set_state \
  robot_dog_interfaces/srv/SetRobotState \
  "{target_state: SIT}"
```

Service不像Topic那样使用`ros2 topic echo`观察数据，应该通过命令调用或客户端日志检查响应。

### 4. 系统

```bash
ros2 doctor
```

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day3.sh
```

测试覆盖接口生成、服务发现、合法状态切换、非法状态拒绝，以及C++静态检查。

## 研发流程记录

### 输入

- 第二天的`robot_dog_interfaces`消息接口包。
- 机器人需要接受一次性状态切换命令的需求。

### 输出

- `SetRobotState.srv`自定义Service接口。
- 状态服务端和客户端。
- 服务端状态校验和明确的进程返回码。

### 验证方式

- `colcon build --packages-up-to robot_dog_day3_services`。
- `ros2 interface show`和`ros2 service type`。
- C++客户端发送合法/非法请求。
- `scripts/test_day3.sh`。

## 实践任务

### 任务1：增加状态转换约束

修改服务端，只允许以下转换：

```text
STANDING -> SIT/TROT/CRAWL
SIT -> STANDING
CRAWL -> STANDING/TROT
TROT -> STANDING/CRAWL
```

其他转换返回失败，并说明原因。

### 任务2：增加查询Service

设计`GetRobotState.srv`，不带请求字段，返回当前状态和电池信息。比较它与第二天状态Topic的区别。

### 任务3：验证服务不可用

不启动服务端，直接运行客户端：

```bash
ros2 run robot_dog_day3_services state_client \
  --ros-args -p timeout_ms:=1000
```

确认客户端不会无限等待，并解释这对机器人启动流程的意义。

## 下一步

第四天学习Action通信：理解长时间任务、反馈和取消，并实现步态执行Action。

---

**学习时间**: 第3天  
**预计用时**: 2-3小时  
**难度**: 基础工程实践
