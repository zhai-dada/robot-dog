# 第一天：ROS2基础 - 第一个节点

## 今日目标

1. 理解ROS2架构和核心概念
2. 创建第一个ROS2工作空间
3. 编写第一个Python节点
4. 编写第一个C++节点
5. 掌握基本的ROS2命令

## 背景知识

### 为什么需要ROS2？

在机器人开发中，我们面临以下挑战：
- **多进程通信**：机器人系统由多个模块组成（感知、控制、决策）
- **硬件抽象**：需要支持不同类型的传感器和执行器
- **算法复用**：避免重复实现常见算法
- **跨平台**：支持不同操作系统和硬件平台

ROS2（Robot Operating System 2）是一个机器人软件框架，它提供了：
- 标准化的通信机制
- 丰富的工具链
- 大量的开源库
- 硬件抽象层

### ROS2与Linux的联系

作为嵌入式工程师，你可以这样理解ROS2概念：

| ROS2概念 | Linux类似概念 | 说明 |
|----------|---------------|------|
| Node | 进程 | 每个节点是一个独立的进程 |
| Topic | IPC管道 | 用于进程间通信 |
| Service | RPC调用 | 请求-响应模式 |
| Parameter | 配置文件 | 运行时参数配置 |
| Launch | 启动脚本 | 批量启动多个节点 |

## 原理

### ROS2架构

```
ROS2系统
├── DDS（数据分发服务）
│   ├── 底层通信
│   ├── 发现机制
│   └── QoS策略
├── ROS2中间件
│   ├── 节点管理
│   ├── 话题管理
│   └── 服务管理
└── 应用层
    ├── 用户节点
    ├── 算法库
    └── 工具链
```

### 节点（Node）

节点是ROS2中的基本计算单元：
- 每个节点是一个独立的进程
- 节点通过话题、服务、动作进行通信
- 节点可以动态创建和销毁

### 话题（Topic）

话题是ROS2中的异步通信机制：
- 发布者（Publisher）发送消息
- 订阅者（Subscriber）接收消息
- 消息类型必须匹配
- 支持多对多通信

## 与Linux嵌入式联系

### 进程 vs 节点

在Linux中，我们使用`fork()`创建进程：
```c
pid_t pid = fork();
if (pid == 0) {
    // 子进程
}
```

在ROS2中，我们创建节点：
```python
node = Node('my_node')
```

两者都是独立的执行单元，但ROS2节点提供了更多的通信能力。

### IPC vs Topic

Linux IPC方式：
- 管道（pipe）
- 消息队列（message queue）
- 共享内存（shared memory）
- 套接字（socket）

ROS2 Topic类似发布-订阅模式的IPC，但更高级：
- 自动发现
- 类型检查
- QoS策略

## 工程实现

### 项目结构

```
robot-dog/
├── src/
│   ├── robot_dog_basics/          # Python节点包
│   │   ├── robot_dog_basics/
│   │   │   ├── __init__.py
│   │   │   └── hello_world_node.py
│   │   ├── package.xml
│   │   ├── setup.py
│   │   └── setup.cfg
│   ├── robot_dog_basics_cpp/      # C++节点包
│   │   ├── src/
│   │   │   └── hello_world_node.cpp
│   │   ├── include/
│   │   ├── CMakeLists.txt
│   │   └── package.xml
│   └── robot_dog_day1_exercises/  # C++练习包
│       ├── src/
│       │   └── robot_dog_day1_exercises.cpp
│       ├── CMakeLists.txt
│       └── package.xml
├── docs/
│   └── README_day1.md              # 第一天教学文档
├── scripts/
│   └── test_day1.sh                # 第一天验收脚本
├── install/                       # 编译输出
├── build/                         # 编译中间文件
└── log/                           # 编译日志
```

### 文件说明

1. **hello_world_node.py** (Python节点)
   - 路径: `src/robot_dog_basics/robot_dog_basics/hello_world_node.py`
   - 作用: 演示Python节点结构
   - 特点: 开发快速，适合原型验证

2. **hello_world_node.cpp** (C++节点)
   - 路径: `src/robot_dog_basics_cpp/src/hello_world_node.cpp`
   - 作用: 演示C++节点结构
   - 特点: 性能高，适合核心算法

3. **robot_dog_day1_exercises.cpp** (C++练习节点)
   - 路径: `src/robot_dog_day1_exercises/src/robot_dog_day1_exercises.cpp`
   - 作用: 综合完成第一天的节点修改和倒计时练习
   - 可执行文件: `counter_node`

## 代码

### Python节点代码

```python
#!/usr/bin/env python3
"""A minimal ROS 2 node example.

This module demonstrates the basic structure of a timer-driven node.
"""

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node


class HelloWorldNode(Node):
    """A minimal timer-driven ROS 2 node."""

    def __init__(self):
        """Initialize the node."""
        super().__init__('my_first_node')

        self.timer = self.create_timer(2.0, self.timer_callback)

        # 计数器
        self.count = 0

        self.get_logger().info('机器人狗系统工程')

    def timer_callback(self):
        """Log a message each time the timer fires."""
        self.get_logger().info(f'Hello World! 计数: {self.count}')
        self.count += 1


def main(args=None):
    """Run the node until ROS 2 shuts down."""
    rclpy.init(args=args)

    node = HelloWorldNode()

    try:
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
```

### C++节点代码

```cpp
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

class HelloWorldNode : public rclcpp::Node
{
public:
  HelloWorldNode()
  : Node("my_first_cpp_node"), count_(0), message_("机器人狗系统工程")
  {
    timer_ = this->create_wall_timer(
      std::chrono::seconds(1),
      std::bind(&HelloWorldNode::timer_callback, this));

    RCLCPP_INFO(
      this->get_logger(), "Hello World C++节点已启动! %s", message_.c_str());
  }

private:
  void timer_callback()
  {
    RCLCPP_INFO(this->get_logger(), "Hello World! 计数: %d", count_++);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  int count_;
  std::string message_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(std::make_shared<HelloWorldNode>());
  } catch (const std::exception & e) {
    RCLCPP_ERROR(rclcpp::get_logger("main"), "节点异常: %s", e.what());
  }

  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }

  return 0;
}
```

## 编译运行

### 编译工作空间

```bash
# 进入工作空间
cd /home/zhai/robot-dog

# 编译所有包
colcon build

# 或者编译特定包
colcon build --packages-select robot_dog_basics
colcon build --packages-select robot_dog_basics_cpp
```

### 运行节点

```bash
# 加载工作空间环境
source install/setup.bash

# 运行Python节点
ros2 run robot_dog_basics hello_world_node

# 运行C++节点
ros2 run robot_dog_basics_cpp hello_world_node

# 运行C++倒计时练习
ros2 run robot_dog_day1_exercises counter_node
```

### 测试命令

```bash
# 查看运行中的节点
ros2 node list

# 查看话题列表
ros2 topic list

# 查看节点信息
ros2 node info /my_first_node

# 查看话题信息
ros2 topic info /topic_name
```

## 调试方法

### 常见问题

1. **编译失败**
   - 检查package.xml依赖
   - 检查CMakeLists.txt配置
   - 检查依赖包是否安装

2. **节点无法运行**
   - 检查环境变量：`echo $ROS_DISTRO`
   - 加载工作空间：`source install/setup.bash`
   - 检查节点是否注册：`ros2 node list`

3. **通信失败**
   - 检查话题名称是否一致
   - 检查消息类型是否匹配
   - 检查QoS设置

### 调试工具

```bash
# ROS2医生，检查系统状态
ros2 doctor

# 查看节点详细信息
ros2 node info /my_first_node

# 查看话题详细信息
ros2 topic info /topic_name --verbose

# 查看消息类型
ros2 topic echo /topic_name
```

## 实践任务

### 任务1：修改Python节点

修改`hello_world_node.py`，实现以下功能：
1. 修改节点名称为`my_first_node`
2. 修改消息内容为`机器人狗系统工程`
3. 修改定时器间隔为2秒

### 任务2：修改C++节点

修改`hello_world_node.cpp`，实现以下功能：
1. 修改节点名称为`my_first_cpp_node`
2. 添加一个成员变量`std::string message_`
3. 在构造函数中初始化`message_`为`机器人狗系统工程`

### 任务3：创建新节点

创建一个新的Python节点`counter_node`，实现：
1. 每0.5秒打印一次计数
2. 计数从100开始递减
3. 当计数到0时停止

本次练习使用C++实现，代码位于
`src/robot_dog_day1_exercises/src/robot_dog_day1_exercises.cpp`，运行命令为：

```bash
ros2 run robot_dog_day1_exercises counter_node
```

### 第一日验收结果

- Python节点已改为`my_first_node`，定时器周期为2秒。
- C++节点已改为`my_first_cpp_node`，并增加`message_`成员。
- C++倒计时节点从100递减到0后主动停止。
- C++包的`cppcheck`、`lint_cmake`、`uncrustify`和`xmllint`检查通过。

## 下一步

第二天将学习：
1. ROS2话题通信（发布者和订阅者）
2. 自定义消息类型
3. 多节点通信
4. 参数配置

### 预习内容

1. 阅读ROS2官方文档：https://docs.ros.org/en/humble/Tutorials.html
2. 了解消息类型定义格式
3. 思考机器人系统中哪些模块需要相互通信

---

**学习时间**: 第1天  
**预计用时**: 2-3小时  
**难度**: 入门
