# 第二天：ROS2话题通信

## 今日目标

1. 理解ROS2 Topic的发布/订阅模型。
2. 使用C++实现Publisher和Subscriber。
3. 创建并使用项目自己的ROS2消息类型。
4. 使用参数控制节点行为。
5. 使用ROS2命令检查节点、Topic、消息类型和QoS。

## 背景知识

第一天的节点只有自己的定时器和日志，节点之间还没有数据交换。真实四足机器人至少需要下面这些模块互相通信：

```text
传感器节点  ->  状态估计节点  ->  控制器节点  ->  执行器接口
                    |
                    +-> 监控和诊断节点
```

例如，IMU节点发布姿态数据，状态估计节点订阅它；控制器再发布关节目标，硬件接口订阅这些目标。Topic就是ROS2中适合连续数据流的通信方式。

## 原理

### Topic的基本组成

```text
Publisher Node  -- RobotStatus message -->  /robot_dog/status  <-- Subscriber Node
```

- **Publisher**：构造并发布消息，不需要知道谁会接收。
- **Subscriber**：声明感兴趣的Topic和消息类型，收到消息后执行回调。
- **Topic**：按名称和消息类型识别的数据通道。
- **DDS**：负责节点发现、数据传输和QoS协商。

Publisher和Subscriber不直接持有对方的对象指针，这种解耦让传感器、控制器和诊断程序可以独立启动、替换和扩展。

### 与Linux嵌入式联系

| ROS2概念 | Linux/嵌入式类比 | 工程含义 |
| --- | --- | --- |
| Node | 进程 | 独立的执行和故障边界 |
| Topic | 发布/订阅消息队列 | 连续、异步的数据流 |
| Message | 有明确布局的IPC数据结构 | 编译期生成类型安全接口 |
| Parameter | sysfs、配置文件或驱动参数 | 启动时注入运行配置 |
| QoS | 缓冲区、丢包和可靠性策略 | 根据数据重要性选择传输行为 |

与裸机环形缓冲区或字符设备相比，ROS2 Topic额外提供了跨进程发现、类型支持和网络传输能力。代价是需要理解DDS匹配规则和QoS，而不是把它当成无成本的全局变量。

## 接口设计

### 自定义消息

文件：`src/robot_dog_interfaces/msg/RobotStatus.msg`

```text
string robot_name
string state
uint64 sequence
float32 battery_percentage
```

字段选择对应机器人状态监控的最小闭环：

- `robot_name`：区分机器人实例。
- `state`：当前状态，例如`STANDING`或`TROT`。
- `sequence`：检查消息是否持续更新、是否出现异常跳变。
- `battery_percentage`：演示带单位约束的数值字段。

接口包独立于业务节点。消息变化时，ROS2会重新生成C/C++/Python类型支持代码，所有消费者都能从同一个接口定义重新编译。

### QoS选择

示例使用`rclcpp::QoS(10)`，表示保留深度为10的队列，并使用默认的可靠性和易失性策略。它适合本地教学演示。

实际机器人中需要按数据性质选择：

- 激光扫描通常更关注最新数据，可以使用较小队列或传感器QoS。
- 控制命令通常需要可靠传输和明确的生命周期。
- 高频关节状态需要评估延迟、丢包和实时线程负担。

QoS不匹配时，节点可能都在运行，但Subscriber收不到消息，这是调试Topic时必须检查的项目。

## 工程实现

### 文件路径和作用

1. `src/robot_dog_interfaces/msg/RobotStatus.msg`
   - 定义共享消息字段。
2. `src/robot_dog_interfaces/CMakeLists.txt`
   - 调用`rosidl_generate_interfaces`生成消息代码。
3. `src/robot_dog_day2_topics/src/status_publisher.cpp`
   - C++状态发布节点。
   - 从参数读取机器人名称、状态、发布周期和电量。
4. `src/robot_dog_day2_topics/src/status_subscriber.cpp`
   - C++状态订阅节点。
   - 在回调中输出收到的结构化消息。
5. `src/robot_dog_day2_topics/config/status_publisher.yaml`
   - 保存可复用的启动参数。
6. `scripts/test_day2.sh`
   - 自动构建、检查接口并验证发布/订阅链路。

### 依赖关系

```text
robot_dog_interfaces
        |
        +--> robot_dog_day2_topics/status_publisher
        |
        +--> robot_dog_day2_topics/status_subscriber
```

`robot_dog_day2_topics/package.xml`声明了`rclcpp`和`robot_dog_interfaces`依赖，CMake通过`find_package`和`ament_target_dependencies`将它们接入目标。

## 代码

### Publisher关键结构

```cpp
publisher_ = this->create_publisher<robot_dog_interfaces::msg::RobotStatus>(
  "robot_dog/status", rclcpp::QoS(10));

timer_ = this->create_wall_timer(
  std::chrono::milliseconds(publish_period_ms_),
  std::bind(&StatusPublisher::publishStatus, this));
```

定时器只负责调度，消息构造和发布在`publish_status()`中完成：

```cpp
robot_dog_interfaces::msg::RobotStatus message;
message.robot_name = robot_name_;
message.state = state_;
message.sequence = sequence_++;
message.battery_percentage = static_cast<float>(battery_percentage_);
publisher_->publish(message);
```

### Subscriber关键结构

```cpp
subscription_ = this->create_subscription<robot_dog_interfaces::msg::RobotStatus>(
  "robot_dog/status",
  rclcpp::QoS(10),
  std::bind(
    &StatusSubscriber::status_callback,
    this,
    std::placeholders::_1));
```

回调收到的是ROS2生成的强类型消息：

```cpp
void statusCallback(
  const robot_dog_interfaces::msg::RobotStatus::ConstSharedPtr message)
{
  RCLCPP_INFO(this->get_logger(), "state=%s", message->state.c_str());
}
```

## 编译运行

### 编译

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day2_topics
source install/setup.bash
```

`--packages-up-to`会先构建`robot_dog_interfaces`，再构建依赖它的Topic包。

### 检查消息定义

```bash
ros2 interface show robot_dog_interfaces/msg/RobotStatus
```

预期输出：

```text
string robot_name
string state
uint64 sequence
float32 battery_percentage
```

### 启动订阅者

终端一：

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 run robot_dog_day2_topics status_subscriber
```

### 启动发布者

终端二：

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 run robot_dog_day2_topics status_publisher
```

### 使用参数文件

```bash
ros2 run robot_dog_day2_topics status_publisher \
  --ros-args \
  --params-file install/robot_dog_day2_topics/share/robot_dog_day2_topics/config/status_publisher.yaml
```

也可以在命令行覆盖参数：

```bash
ros2 run robot_dog_day2_topics status_publisher \
  --ros-args \
  -p robot_name:=sim_dog \
  -p state:=TROT \
  -p battery_percentage:=87.5 \
  -p publish_period_ms:=200
```

## 调试方法

按照项目规定的顺序检查：

### 1. 节点

```bash
ros2 node list
ros2 node info /status_publisher
ros2 node info /status_subscriber
```

### 2. Topic

```bash
ros2 topic list
ros2 topic info /robot_dog/status
ros2 topic info /robot_dog/status --verbose
```

### 3. 数据

```bash
ros2 topic type /robot_dog/status
ros2 topic hz /robot_dog/status
ros2 topic echo /robot_dog/status
```

### 4. 系统

```bash
ros2 doctor
```

### 常见故障定位

| 现象 | 优先检查 |
| --- | --- |
| `Package not found` | 是否执行了`source install/setup.bash` |
| Topic列表没有`/robot_dog/status` | Publisher是否启动、ROS_DOMAIN_ID是否一致 |
| Topic存在但没有数据 | Publisher/Subscriber消息类型和QoS是否匹配 |
| 自定义消息找不到 | 先构建接口包，再source工作空间 |
| 参数未生效 | 参数文件中的节点名是否为`status_publisher` |

## 测试方法

自动测试：

```bash
cd /home/zhai/robot-dog
./scripts/test_day2.sh
```

测试覆盖：

- 接口包生成和`ros2 interface show`。
- C++包编译。
- Publisher和Subscriber能否匹配同一Topic。
- 自定义消息字段是否正确传输。
- 参数文件中的机器人名称、状态和电量是否生效。
- C++静态检查。

## 实践任务

### 任务1：增加状态字段

为`RobotStatus.msg`增加一个字段，例如：

```text
float32 temperature_celsius
```

要求：

1. 更新Publisher赋值。
2. 更新Subscriber输出。
3. 重新编译接口包和Topic包。
4. 使用`ros2 topic echo`验证。

### 任务2：实现多个订阅者

创建一个C++诊断节点，只在电量低于30%时输出WARN日志。不要修改现有Subscriber。

### 任务3：验证参数边界

分别传入以下参数并记录结果：

```bash
-p publish_period_ms:=0
-p battery_percentage:=-1.0
-p battery_percentage:=101.0
```

解释为什么节点应拒绝这些配置。

### 任务4：观察消息频率

```bash
ros2 topic hz /robot_dog/status
```

分别使用500毫秒和200毫秒周期，比较实测频率与理论频率，并说明调度误差来源。

## 研发流程记录

### 输入

- 第一天已经验证的ROS2节点和C++包结构。
- 机器人状态监控需要共享数据的需求。

### 输出

- 独立的接口包`robot_dog_interfaces`。
- 结构化消息`RobotStatus`。
- 一个Publisher和一个Subscriber。
- 可复用参数文件和自动化测试脚本。

### 验证方式

- `colcon build --packages-up-to robot_dog_day2_topics`。
- `ros2 interface show robot_dog_interfaces/msg/RobotStatus`。
- `ros2 topic info`、`ros2 topic echo`和`ros2 topic hz`。
- `scripts/test_day2.sh`。

## 下一步

第三天学习Service通信：

1. 理解请求/响应与Topic异步流的差异。
2. 使用C++实现Service Server和Client。
3. 为机器人增加“切换状态”服务。
4. 学习何时应该使用Service，何时应该使用Topic。

---

**学习时间**: 第2天  
**预计用时**: 2-3小时  
**难度**: 入门到基础工程实践
