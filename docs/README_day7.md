# 第七天：ROS2 QoS策略

## 今日目标

1. 理解QoS为什么会影响Publisher与Subscriber能否通信。
2. 掌握Reliability、Durability、History和Depth四个基础策略。
3. 使用C++参数构造不同的`rclcpp::QoS`配置。
4. 使用QoS事件回调定位“Topic存在但收不到数据”的问题。
5. 理解本次新增的C++语法、函数签名、Lambda和ROS2 API。

## 背景知识

前面的Topic示例中，发布者与订阅者都使用：

```cpp
rclcpp::QoS(10)
```

这会使用默认的Reliable、Volatile和KeepLast策略。简单示例中双方配置相同，因此通信正常。但是机器人传感器、控制命令和地图数据对丢包、延迟、缓存的要求不同，不能全部使用同一配置。

典型情况：

```text
LiDAR:      高频、允许少量丢包、优先低延迟
控制命令:  希望可靠送达，但不能无限积压旧命令
静态地图:  新订阅者启动后仍希望获得最近地图
相机图像:  数据量大，通常更关注最新帧
```

QoS是Quality of Service的缩写，用来描述DDS通信的服务质量约束。

## 原理

### Request与Offer模型

Publisher提供一组QoS能力，Subscriber提出一组QoS要求：

```text
Publisher: Offered QoS
Subscriber: Requested QoS
```

只有Publisher提供的能力满足Subscriber要求时，DDS才建立数据连接。

### Reliability可靠性

#### Reliable

发送端尽力保证消息送达，必要时重传。

适合：

- 低频控制指令
- 配置消息
- 不能轻易丢失的状态变化

代价：网络不稳定或Subscriber处理过慢时，可能增加延迟和缓存压力。

#### Best Effort

尽力发送，但不保证重传。消息丢失后继续发送新消息。

适合：

- LiDAR
- 相机图像
- 高频IMU
- 更关注最新值的传感器流

#### 兼容关系

| Publisher | Subscriber | 是否兼容 |
| --- | --- | --- |
| Reliable | Reliable | 是 |
| Reliable | Best Effort | 是 |
| Best Effort | Best Effort | 是 |
| Best Effort | Reliable | 否 |

原因是Reliable Publisher提供的能力高于Best Effort要求；Best Effort Publisher无法满足Reliable Subscriber的重传要求。

### Durability持久性

#### Volatile

Subscriber只能收到建立连接后发布的新消息，无法获得历史缓存。

#### Transient Local

Publisher在自身进程内保存历史消息。晚启动Subscriber如果使用兼容QoS，可以获得缓存消息。

注意：缓存属于Publisher进程。Publisher进程退出后，Transient Local缓存通常也消失，它不是数据库或磁盘持久化。

适合：

- 静态地图
- 机器人描述
- 低频但新节点启动后必须获得的状态

### History与Depth

#### Keep Last

只保存最近N条消息：

```cpp
rclcpp::KeepLast(10)
```

当第11条消息到来时，最旧消息会被丢弃。

#### Keep All

尝试保存所有尚未处理的消息：

```cpp
rclcpp::KeepAll()
```

Keep All仍然受DDS和系统资源限制。机器人高频数据不应无条件使用Keep All，否则可能造成内存增长和延迟累积。

### 与Linux嵌入式联系

| ROS2 QoS | Linux/嵌入式类比 |
| --- | --- |
| Reliability | TCP可靠传输与UDP尽力传输的取舍 |
| KeepLast Depth | 环形缓冲区深度 |
| KeepAll | 无固定长度队列，但仍受内存限制 |
| Volatile | 打开设备后只接收新事件 |
| Transient Local | 驱动保存最近状态供后来读取 |
| QoS事件回调 | 网络错误、队列溢出或驱动错误通知 |

QoS不是简单等同TCP或UDP，因为ROS2 DDS可以独立组合可靠性、历史、持久性、Deadline和Liveliness等策略。

## 工程实现

### 文件路径

1. `src/robot_dog_day7_qos/src/qos_status_publisher.cpp`
   - 参数化QoS Publisher。
   - 发布`RobotStatus`消息。
   - 注册Offered QoS不兼容事件。
2. `src/robot_dog_day7_qos/src/qos_status_subscriber.cpp`
   - 参数化QoS Subscriber。
   - 注册Requested QoS不兼容事件。
3. `src/robot_dog_day7_qos/config/qos_publisher.yaml`
   - Publisher默认参数。
4. `src/robot_dog_day7_qos/config/qos_subscriber.yaml`
   - Subscriber默认参数。
5. `scripts/test_day7.sh`
   - 自动执行兼容、不兼容和晚加入实验。

### 依赖

```text
rclcpp
robot_dog_interfaces
```

`robot_dog_interfaces`提供`RobotStatus`消息，`rclcpp`提供Node、Publisher、Subscriber、QoS和事件回调API。

## C++基础语法说明

### 1. const成员函数

```cpp
rclcpp::QoS createQos() const
```

最后的`const`表示该函数不修改当前对象的普通成员变量。它不是说返回值不可修改。

这样做的意义：

- 编译器帮助检查函数是否意外修改对象状态。
- 调用者能明确知道这是一个只读取配置的函数。
- `const`对象只能调用`const`成员函数。

### 2. 三元运算符

```cpp
rclcpp::QoS qos = history_ == "keep_all" ?
  rclcpp::QoS(rclcpp::KeepAll()) :
  rclcpp::QoS(rclcpp::KeepLast(depth_));
```

语法：

```text
条件 ? 条件为真时的值 : 条件为假时的值
```

这里根据字符串参数选择KeepAll或KeepLast。两侧都返回`rclcpp::QoS`对象，因此可以赋值给同一个变量。

### 3. auto类型推导

```cpp
const auto qos = createQos();
```

编译器从`createQos()`返回值推导`qos`类型为`rclcpp::QoS`。

适合使用`auto`的情况：

- 类型很长但从右侧明显可知。
- 迭代器、智能指针或模板返回类型。

不应为了少写几个字符而隐藏关键业务类型。

### 4. Lambda表达式

```cpp
[this](rclcpp::QOSRequestedIncompatibleQoSInfo & info)
{
  RCLCPP_WARN(this->get_logger(), "QoS不兼容");
};
```

Lambda是匿名函数。

- `[this]`：捕获当前对象指针，因此Lambda里能访问`this->get_logger()`。
- `(Info & info)`：参数列表，`&`表示引用，避免复制事件结构体。
- `{ ... }`：函数体。
- `;`：Lambda被赋值给回调对象，因此表达式末尾需要分号。

它等价于写一个专用成员函数再用`std::bind`绑定，但短回调使用Lambda更直接。

### 5. 智能指针类型

```cpp
rclcpp::Publisher<RobotStatus>::SharedPtr publisher_;
```

`SharedPtr`本质上是`std::shared_ptr`别名。ROS2实体可能被Node、Executor和用户代码共同持有，因此使用引用计数管理生命周期。

当最后一个`shared_ptr`销毁时，对象才会释放。

### 6. 链式调用

```cpp
qos.reliable().transient_local();
```

QoS配置函数返回`QoS &`，也就是当前对象的引用，因此可以连续调用。下面两种写法等价：

```cpp
qos.reliable();
qos.transient_local();
```

```cpp
qos.reliable().transient_local();
```

本项目代码使用分开的`if`语句，是为了让初学阶段更容易观察每个策略来源。

### 7. static_cast显式转换

```cpp
static_cast<int>(info.last_policy_kind)
```

`last_policy_kind`不是普通`int`，打印时显式转换可以避免隐式类型转换不明确。`static_cast`用于编译期可检查的常规类型转换。

## ROS2 API说明

### 1. `rclcpp::KeepLast`

```cpp
rclcpp::KeepLast(depth_)
```

构造一个QoS初始化对象：

- 参数：`depth`，类型是`size_t`。
- 含义：最多保存最近多少条消息。
- 返回：`rclcpp::KeepLast`临时对象。

### 2. `rclcpp::KeepAll`

```cpp
rclcpp::KeepAll()
```

构造Keep All历史策略，不需要深度参数。

### 3. `rclcpp::QoS`

```cpp
rclcpp::QoS qos(rclcpp::KeepLast(10));
```

`QoS`对象封装DDS QoS配置。常用构造方式：

```cpp
rclcpp::QoS(10)
rclcpp::QoS(rclcpp::KeepLast(10))
rclcpp::QoS(rclcpp::KeepAll())
```

第一种是KeepLast的便捷写法。

### 4. `reliable()`与`best_effort()`

```cpp
qos.reliable();
qos.best_effort();
```

两者修改QoS对象的Reliability策略，返回当前`QoS &`。

### 5. `transient_local()`与`durability_volatile()`

```cpp
qos.transient_local();
qos.durability_volatile();
```

`volatile`是C++关键字，所以ROS2 API不能直接命名为`volatile()`，因此使用`durability_volatile()`。

### 6. `rclcpp::PublisherOptions`

```cpp
rclcpp::PublisherOptions options;
```

Publisher附加配置对象。除了QoS本身，还可配置事件回调、Callback Group和内存分配器。

本次使用：

```cpp
options.event_callbacks.incompatible_qos_callback = ...;
```

当Publisher发现Subscriber要求无法满足时，DDS触发该回调。

### 7. `rclcpp::SubscriptionOptions`

与PublisherOptions类似，但用于Subscriber。本次注册的是Requested QoS不兼容回调。

### 8. `create_publisher`

```cpp
publisher_ = this->create_publisher<RobotStatus>(
  "robot_dog/qos_status",
  qos,
  options);
```

模板参数：

- `RobotStatus`：消息类型。

函数参数：

1. Topic名称。
2. QoS对象。
3. PublisherOptions。

返回值：Publisher的`SharedPtr`。

### 9. `create_subscription`

```cpp
subscription_ = this->create_subscription<RobotStatus>(
  topic_name,
  qos,
  callback,
  options);
```

函数参数：

1. Topic名称。
2. Subscriber请求的QoS。
3. 收到消息后的回调函数。
4. SubscriptionOptions。

### 10. QoS不兼容信息

```cpp
rclcpp::QOSRequestedIncompatibleQoSInfo
rclcpp::QOSOfferedIncompatibleQoSInfo
```

常用字段：

- `total_count`：累计发生不兼容的次数。
- `total_count_change`：本次事件新增次数。
- `last_policy_kind`：最近导致不兼容的QoS策略类型。

具体数值由底层RMW/DDS定义。调试时先确认发生不兼容，再用`ros2 topic info --verbose`查看双方QoS配置。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day7_qos
source install/setup.bash
```

### 实验一：Reliable到Best Effort

终端一：

```bash
ros2 run robot_dog_day7_qos qos_status_subscriber \
  --ros-args -p reliability:=best_effort
```

终端二：

```bash
ros2 run robot_dog_day7_qos qos_status_publisher \
  --ros-args -p reliability:=reliable
```

Subscriber应能收到消息。

### 实验二：Best Effort到Reliable

终端一：

```bash
ros2 run robot_dog_day7_qos qos_status_subscriber \
  --ros-args -p reliability:=reliable
```

终端二：

```bash
ros2 run robot_dog_day7_qos qos_status_publisher \
  --ros-args -p reliability:=best_effort
```

Subscriber不会收到业务消息，双方会输出QoS不兼容警告。

### 实验三：Transient Local晚加入

先启动只发布一条消息的Publisher：

```bash
ros2 run robot_dog_day7_qos qos_status_publisher \
  --ros-args \
  -p durability:=transient_local \
  -p publish_count:=1
```

看到“停止发布并保持节点存活”后，再启动Subscriber：

```bash
ros2 run robot_dog_day7_qos qos_status_subscriber \
  --ros-args -p durability:=transient_local
```

晚启动Subscriber仍应收到`sequence=0`。

## 调试方法

按照ROS2调试顺序：

```bash
ros2 node list
ros2 topic list
ros2 topic info /robot_dog/qos_status --verbose
ros2 topic echo /robot_dog/qos_status
ros2 doctor
```

重点查看`ros2 topic info --verbose`中的：

```text
Reliability
Durability
History
Depth
```

Topic存在但没有数据时，不能立即修改业务回调。先比较Publisher和Subscriber的QoS配置。

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day7.sh
```

测试覆盖：

- Reliable Publisher与Best Effort Subscriber兼容。
- Best Effort Publisher与Reliable Subscriber不兼容。
- 不兼容事件回调被触发。
- Transient Local支持晚加入Subscriber。
- C++构建和静态检查。

## 实践任务

1. 使用`ros2 topic info --verbose`记录三组实验的QoS输出。
2. 把`depth`分别设置为1、5、20，设计一个慢Subscriber观察队列变化。
3. 使用`history:=keep_all`运行高频Publisher，解释为什么工业代码必须设置资源限制。
4. 将第二天状态Topic改成参数化QoS，并说明默认应选择哪种策略。
5. 查阅`sensor_data` QoS，比较它与`rclcpp::QoS(10)`的区别。

## 研发流程记录

### 输入

- 已完成的Publisher和Subscriber基础。
- 机器人不同数据流对可靠性和延迟的不同需求。

### 输出

- 参数化QoS Publisher和Subscriber。
- QoS不兼容事件回调。
- Reliability、Durability和History实验。

### 验证方式

- `ros2 topic info --verbose`。
- 三组手工实验。
- `scripts/test_day7.sh`。

## 下一步

第八天学习Lifecycle Node，建立`unconfigured -> inactive -> active -> finalized`生命周期，并把传感器或控制器资源创建与启停绑定到明确状态。
