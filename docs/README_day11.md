# 第十一天：C++工程分层入门

## 今日目标

1. 理解机器人软件中的工具库、驱动层和控制层。
2. 使用C++17创建可复用库。
3. 让控制节点和模拟驱动通过Topic连接。
4. 理解头文件、实现文件、库和可执行文件的区别。
5. 为后续运动学和步态控制建立包边界。

## 背景知识

前十天主要学习ROS2通信机制。第二阶段开始关注机器人软件本身如何分层：

```text
robot_control       控制算法和关节目标
        |
        v
robot_drivers       电机/传感器硬件抽象
        |
        v
real hardware       串口、CAN、I2C、SPI

robot_utils         被多个层复用的数学和通用工具
```

本日不接真实硬件，使用模拟驱动验证架构边界。这样后续把模拟驱动替换成CAN或串口驱动时，控制层接口不必全部重写。

## 与Linux嵌入式联系

| robot-dog包 | Linux嵌入式对应层 |
| --- | --- |
| `robot_utils` | 公共静态库、BSP工具库 |
| `robot_drivers` | 设备驱动和HAL |
| `robot_control` | 用户态控制算法/设备控制服务 |
| Topic接口 | 用户态进程间IPC |
| CMake依赖 | 链接库和头文件依赖 |

驱动层不应该把串口细节泄露给导航和控制算法。控制层也不应该直接操作`/dev/ttyUSB0`。

## 工程实现

### 文件路径

1. `src/robot_utils/include/robot_utils/angle_utils.hpp`
   - 公共工具库接口。
2. `src/robot_utils/src/angle_utils.cpp`
   - 工具库实现。
3. `src/robot_control/src/joint_command_publisher.cpp`
   - 发布三个模拟关节目标。
4. `src/robot_drivers/src/fake_motor_driver.cpp`
   - 订阅关节目标，执行范围限制并模拟输出到电机。
5. `scripts/test_day11.sh`
   - 验证控制层到驱动层的链路。

### 包依赖

```text
robot_utils
    ^             ^
    |             |
robot_control  robot_drivers
```

`robot_control`和`robot_drivers`都依赖`robot_utils`，但两者不直接依赖对方。它们通过ROS2 Topic解耦。

## C++基础语法说明

### 1. 头文件声明和实现文件

头文件只声明接口：

```cpp
class AngleUtils
{
public:
  static double clamp(double value, double minimum, double maximum);
};
```

实现文件提供函数体：

```cpp
{
  return std::clamp(value, minimum, maximum);
}
```

`AngleUtils::`表示该函数属于`robot_utils::AngleUtils`类。

### 2. `static`成员函数

```cpp
robot_utils::AngleUtils::clamp(value, -1.57, 1.57);
```

静态成员函数不依赖某个对象实例，因此可以通过类名直接调用。它不能直接访问非静态成员变量。

### 3. namespace

```cpp
namespace robot_utils
{
// symbols
}
```

命名空间避免公共类名和其他包冲突。调用时使用完整限定名，减少全局命名污染。

### 4. `std::vector`

```cpp
std::vector<double> safe_commands;
safe_commands.reserve(message->data.size());
```

`vector`是连续存储、可动态扩展的数组。`reserve()`提前申请容量，减少循环中重新分配内存。

### 5. 范围for

```cpp
for (const double command : message->data)
```

依次访问容器中的元素。`const double`表示循环变量不会修改原数据。

### 6. initializer list

```cpp
message.data = {value_a, value_b, value_c};
```

使用花括号快速构造多个元素。这里给ROS数组字段赋值三个关节目标。

### 7. CMake库目标

```cmake
add_library(robot_utils SHARED src/angle_utils.cpp)
target_include_directories(robot_utils PUBLIC ...)
```

- `add_library`创建库目标。
- `SHARED`生成动态库。
- `target_include_directories`告诉编译器公共头文件位置。
- `PUBLIC`表示使用该库的下游目标也需要这个头文件目录。

## ROS2 API说明

### 1. `ament_target_dependencies`

```cmake
ament_target_dependencies(fake_motor_driver rclcpp robot_utils std_msgs)
```

把ROS2包导出的头文件、库和编译选项连接到目标。只在`package.xml`写依赖而不在CMake连接，可能导致编译或安装环境失败。

### 2. `create_publisher<T>`

```cpp
publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
  "robot_dog/joint_commands", rclcpp::QoS(10));
```

模板参数`T`是消息类型，运行参数是Topic名和QoS，返回Publisher智能指针。

### 3. `create_subscription<T>`

```cpp
subscription_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
  "robot_dog/joint_commands",
  rclcpp::QoS(10),
  std::bind(&FakeMotorDriver::commandCallback, this, std::placeholders::_1));
```

第三个参数是回调。`std::bind`把成员函数和当前对象绑定起来，`std::placeholders::_1`表示由ROS2在运行时传入消息参数。

### 4. `std::clamp`

```cpp
robot_utils::AngleUtils::clamp(command, -1.57, 1.57);
```

这是C++17标准库函数，保证结果位于上下限之间。驱动层进行最后一道范围保护，避免控制层异常目标直接传到执行器。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_control robot_drivers
source install/setup.bash
```

终端一启动模拟驱动：

```bash
ros2 run robot_drivers fake_motor_driver
```

终端二启动控制节点：

```bash
ros2 run robot_control joint_command_publisher
```

检查Topic：

```bash
ros2 node list
ros2 topic list
ros2 topic echo /robot_dog/joint_commands
```

## 调试方法

```bash
ros2 node info /joint_command_publisher
ros2 node info /fake_motor_driver
ros2 topic info /robot_dog/joint_commands --verbose
ros2 topic echo /robot_dog/joint_commands
ros2 doctor
```

重点检查：

1. 两个节点Topic名称是否一致。
2. 消息类型是否都是`std_msgs/msg/Float64MultiArray`。
3. `robot_utils`是否被正确安装和链接。
4. 驱动是否拒绝长度不为3的数组。
5. 驱动是否把超出范围的目标限制到`[-1.57, 1.57]`。

## 测试方法

```bash
./scripts/test_day11.sh
```

测试覆盖工具库链接、控制节点发布、模拟驱动订阅和关节目标范围保护。

## 研发流程记录

### 输入

- 第一阶段ROS2通信和C++基础。
- 控制层与硬件层需要解耦的需求。

### 输出

- `robot_utils`动态库。
- `robot_control`控制节点。
- `robot_drivers`模拟驱动节点。
- 通过Topic连接的最小控制链路。

### 验证方式

- `colcon build --packages-up-to robot_control robot_drivers`。
- `ros2 node info`和`ros2 topic info`。
- `scripts/test_day11.sh`。

## 实践任务

1. 增加四足机器人12个关节的命令数组，并说明数组顺序如何定义。
2. 创建`MotorCommand.msg`，替代`Float64MultiArray`。
3. 在驱动层增加电机故障状态Topic。
4. 将控制节点的正弦信号替换为固定站立姿态。
5. 解释为什么硬件串口和CAN代码应该继续留在`robot_drivers`中。

## 下一步

第十二天学习CMake、package.xml和依赖管理的深入内容，为运动学库和控制器测试做准备。
