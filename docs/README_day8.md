# 第八天：ROS2 Lifecycle Node

## 今日目标

1. 理解Lifecycle Node与普通Node的区别。
2. 掌握`unconfigured`、`inactive`、`active`和`finalized`状态。
3. 将资源创建、启动、停止和释放绑定到生命周期回调。
4. 使用C++实现Lifecycle Publisher。
5. 使用ROS2命令执行和检查生命周期转换。

## 背景知识

普通Node一旦构造完成，通常就会创建Publisher、Timer并立即运行。如果传感器硬件还没有准备好、参数不合法或控制器尚未完成初始化，节点可能已经开始发布错误数据。

Lifecycle Node把节点运行拆成受控阶段：

```text
unconfigured -- configure --> inactive -- activate --> active
      ^                         |  ^                    |
      |                         |  |                    |
      +-------- cleanup --------+  +------ deactivate --+
```

对于robot-dog：

- `unconfigured`：驱动对象尚未创建。
- `inactive`：资源已创建，但不对外发布业务数据。
- `active`：传感器或控制器正式运行。
- `finalized`：节点不可再使用，准备退出。

这相当于嵌入式系统中的设备状态机：设备存在、设备已初始化、设备运行、设备关闭。

## 与Linux嵌入式联系

| Lifecycle状态 | Linux/嵌入式类比 | robot-dog含义 |
| --- | --- | --- |
| `unconfigured` | 驱动模块已加载但设备未open | 不创建运行资源 |
| `inactive` | 设备已打开但未启动采样 | 保留资源，不发布业务数据 |
| `active` | 设备开始采样/中断工作 | 正式发布传感器数据 |
| `finalized` | close后释放设备 | 不再接受正常工作 |

Lifecycle的价值不是增加状态名称，而是让系统管理器知道节点当前是否真的可用。

## 原理

### 状态和转换

Lifecycle Node由状态和转换组成：

```text
状态：unconfigured / inactive / active / finalized
转换：configure / cleanup / activate / deactivate / shutdown
```

执行转换时，ROS2调用对应的虚函数：

```cpp
on_configure()
on_cleanup()
on_activate()
on_deactivate()
on_shutdown()
on_error()
```

回调返回：

- `SUCCESS`：转换成功，进入目标状态。
- `FAILURE`：转换失败，通常回到原状态。
- `ERROR`：发生错误，进入错误处理流程。

### 资源管理规则

```text
on_configure   创建Publisher、Timer、硬件句柄
on_activate    启用Publisher和业务执行
on_deactivate  停止发布、停止控制输出，但保留资源
on_cleanup     释放Publisher、Timer、硬件句柄
on_shutdown    无论当前阶段如何，都执行最终释放
```

不要在构造函数里创建依赖硬件状态的资源，也不要把“停止业务”和“释放资源”混成一个回调。

### LifecyclePublisher

普通Publisher创建后可以直接发布。LifecyclePublisher则受激活状态保护：

```text
inactive -> publish()被阻止
active   -> publish()真正发送消息
```

这样即使某个定时器意外触发，inactive节点也不会把未准备好的数据发送给其他模块。

## 工程实现

### 文件路径

1. `src/robot_dog_day8_lifecycle/src/lifecycle_status_publisher.cpp`
   - C++ Lifecycle Node示例。
   - `configure`创建资源。
   - `activate`开启发布。
   - `deactivate`停止业务。
   - `cleanup`释放资源。
2. `src/robot_dog_day8_lifecycle/config/lifecycle_status_publisher.yaml`
   - 生命周期节点参数。
3. `scripts/test_day8.sh`
   - 自动执行生命周期转换并验证Topic行为。

### 设计原因

本示例发布已有的`RobotStatus`消息，避免引入新的消息类型。只有节点进入active状态后，才发布：

```text
/robot_dog/lifecycle_status
```

`fail_configuration`参数用于演示配置失败路径；`publish_period_ms`和`battery_percentage`在configure阶段校验。

## C++基础语法说明

### 1. 继承

```cpp
class LifecycleStatusPublisher final
  : public rclcpp_lifecycle::LifecycleNode
```

- `class`定义一个类型。
- `public`表示继承的公开接口保持公开。
- `final`禁止其他类继续继承这个类。
- `:`后面是基类列表。

Lifecycle节点必须继承`rclcpp_lifecycle::LifecycleNode`，才能获得生命周期状态机和生命周期实体接口。

### 2. using类型别名

```cpp
using CallbackReturn =
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
```

`using`为复杂类型建立短名称。后续可以写：

```cpp
CallbackReturn on_configure(...)
```

而不必重复完整命名空间。

### 3. override

```cpp
CallbackReturn on_configure(
  const rclcpp_lifecycle::State & previous_state) override
```

`override`告诉编译器：这个函数必须覆盖基类的虚函数。如果函数名、参数或返回类型不匹配，编译器会报错。

这比依赖函数名称约定更安全，适合生命周期回调这种框架接口。

### 4. 引用和const引用

```cpp
const rclcpp_lifecycle::State & previous_state
```

- `&`表示引用，不复制对象。
- `const`表示函数不能通过该引用修改原对象。

生命周期状态对象很小，但使用const引用表达了“只读取前一状态”的意图。

### 5. 指针布尔判断

```cpp
if (!publisher_ || !timer_)
```

ROS2实体是智能指针。空指针转换为`false`，非空指针转换为`true`。`!`表示逻辑取反，所以这里检查资源是否缺失。

### 6. reset释放智能指针

```cpp
publisher_.reset();
```

`reset()`释放当前智能指针持有的对象引用。如果这是最后一个引用，底层资源会析构。它相当于把指针置空，并触发引用计数减少。

### 7. 初始化列表

```cpp
LifecycleStatusPublisher()
: rclcpp_lifecycle::LifecycleNode("lifecycle_status_publisher")
```

冒号后的初始化列表在构造函数体执行前初始化基类和成员。基类必须在初始化列表中构造，不能在构造函数体里重新构造。

## ROS2 API说明

### 1. `rclcpp_lifecycle::LifecycleNode`

```cpp
LifecycleNode("lifecycle_status_publisher")
```

构造Lifecycle Node。

- 参数：节点名称。
- 返回：构造完成的Lifecycle Node对象。
- 注意：构造完成不代表节点已经active。

### 2. `declare_parameter`

```cpp
this->declare_parameter<int>("publish_period_ms", 500);
```

模板函数声明参数：

1. 模板参数`int`：参数类型。
2. 参数名。
3. 默认值。

返回值是声明后的参数值。这里构造函数只声明参数，实际资源在configure时创建。

### 3. `get_parameter`

```cpp
publish_period_ms_ = this->get_parameter("publish_period_ms").as_int();
```

`get_parameter`返回`rclcpp::Parameter`对象，`as_int()`把它转换为`int64_t`。

常用转换函数：

```cpp
as_string()
as_int()
as_double()
as_bool()
```

参数类型不匹配时会抛出异常，因此调用处应有错误处理。

### 4. `create_publisher`

```cpp
publisher_ = this->create_publisher<RobotStatus>(
  "robot_dog/lifecycle_status",
  rclcpp::QoS(10));
```

LifecycleNode版本返回`LifecyclePublisher<RobotStatus>::SharedPtr`，而普通Node返回普通Publisher指针。模板参数是消息类型，运行参数是Topic名和QoS。

### 5. `LifecyclePublisher::on_activate`

```cpp
publisher_->on_activate();
```

启用LifecyclePublisher内部的发布状态。必须在Lifecycle节点的`on_activate`回调中调用。

### 6. `LifecyclePublisher::on_deactivate`

```cpp
publisher_->on_deactivate();
```

停止LifecyclePublisher的实际发布，但不销毁Publisher对象。

### 7. Timer的`reset`和`cancel`

```cpp
```

- `reset()`启动或重新启动定时器。
- `cancel()`取消定时器触发。

本实现configure后先cancel，activate后reset，deactivate时再次cancel。

### 8. 生命周期回调返回值

```cpp
return CallbackReturn::SUCCESS;
return CallbackReturn::FAILURE;
return CallbackReturn::ERROR;
```

这些是`enum class`枚举值。`enum class`要求使用类型名限定成员，能避免枚举成员污染全局命名空间。

### 9. `rclcpp::spin`

```cpp
rclcpp::spin(node->get_node_base_interface());
```

`spin`让Executor持续处理服务、定时器和生命周期请求。这里传入Node Base Interface，是因为LifecycleNode通过接口体系暴露给Executor。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day8_lifecycle
source install/setup.bash
```

启动节点：

```bash
ros2 run robot_dog_day8_lifecycle lifecycle_status_publisher \
  --ros-args \
  --params-file install/robot_dog_day8_lifecycle/share/robot_dog_day8_lifecycle/config/lifecycle_status_publisher.yaml
```

查看Lifecycle节点：

```bash
ros2 lifecycle nodes
ros2 lifecycle get /lifecycle_status_publisher
ros2 lifecycle list /lifecycle_status_publisher
```

执行配置和激活：

```bash
ros2 lifecycle set /lifecycle_status_publisher configure
ros2 lifecycle set /lifecycle_status_publisher activate
ros2 topic echo /robot_dog/lifecycle_status
```

停止和清理：

```bash
ros2 lifecycle set /lifecycle_status_publisher deactivate
ros2 lifecycle set /lifecycle_status_publisher cleanup
```

## 调试方法

```bash
ros2 node list
ros2 lifecycle nodes
ros2 lifecycle get /lifecycle_status_publisher
ros2 lifecycle list /lifecycle_status_publisher
ros2 topic list
ros2 topic info /robot_dog/lifecycle_status --verbose
ros2 topic echo /robot_dog/lifecycle_status
ros2 doctor
```

按状态排查：

| 状态 | 预期行为 |
| --- | --- |
| `unconfigured` | 没有Publisher和Timer运行资源 |
| `inactive` | 资源存在，但没有业务消息发布 |
| `active` | 周期发布`RobotStatus` |
| `cleanup`后 | 资源被释放，可再次configure |

如果生命周期服务不可用，先检查节点是否是LifecycleNode，而不是普通Node；再检查节点名称和工作空间环境。

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day8.sh
```

测试覆盖：

- 初始`unconfigured`状态。
- configure进入`inactive`。
- inactive状态不发布业务消息。
- activate进入`active`并发布消息。
- deactivate停止发布。
- cleanup释放资源并回到`unconfigured`。
- 非法配置被拒绝。

## 实践任务

1. 增加`on_error`故障计数，并发布诊断消息。
2. 让参数回调只允许在`inactive`状态修改发布周期。
3. 为Lifecycle节点增加一个模拟传感器句柄，在configure打开、cleanup关闭。
4. 比较普通Publisher和LifecyclePublisher在inactive状态下的行为。
5. 设计服务端关闭流程：停止接受新任务、取消Action、停止控制器、释放硬件、最终shutdown。

## 研发流程记录

### 输入

- 第七天QoS实验和已有`RobotStatus`消息。
- 传感器/控制器需要受控初始化和启停的需求。

### 输出

- C++ Lifecycle Node。
- 生命周期资源管理回调。
- LifecyclePublisher及完整状态转换测试。

### 验证方式

- `ros2 lifecycle get/list/set`。
- `ros2 topic echo /robot_dog/lifecycle_status`。
- `scripts/test_day8.sh`。

## 下一步

第九天学习TF坐标变换，建立`base_link`、机身和传感器坐标系之间的关系。
