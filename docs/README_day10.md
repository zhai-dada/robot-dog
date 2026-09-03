# 第十天：ROS2基础阶段综合验收

## 今日目标

1. 复盘ROS2基础通信模型。
2. 明确Topic、Service、Action的边界。
3. 综合使用参数、QoS、Lifecycle和TF。
4. 按真实机器人研发流程执行阶段验收。
5. 形成后续第二阶段C++开发的工程基线。

## 阶段成果

第一阶段已经建立以下能力：

```text
Node       进程和计算单元
Topic      连续异步数据
Service    一次请求和响应
Action     长时间任务、反馈和取消
Parameter  启动及运行时配置
Launch     多节点启动编排
QoS        数据传输质量约束
Lifecycle  节点资源状态管理
TF         坐标系和空间变换
```

## 与Linux嵌入式联系

```text
ROS2 Node      ~= Linux进程
Topic          ~= IPC发布订阅
Service        ~= ioctl/RPC
Action         ~= 异步工作队列
Parameter      ~= sysfs和配置文件
Launch         ~= systemd/启动脚本
QoS            ~= 缓冲区和可靠传输策略
Lifecycle      ~= 设备驱动状态机
TF             ~= 传感器安装外参与坐标变换
```

ROS2不是替代Linux内核和驱动。它位于机器人应用与硬件抽象之间，负责模块组织、通信和工具链。

## 综合验收项目

### 1. 节点和编译

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build
```

### 2. 通信链路

验证：

- Topic状态从Publisher传到Subscriber。
- Service状态切换返回成功或业务拒绝。
- Action步态任务输出Feedback和Result。
- 参数修改触发回调并改变运行行为。

### 3. 系统管理

验证：

- Launch批量启动基础节点。
- QoS兼容和不兼容行为可解释。
- Lifecycle节点只在active状态发布业务数据。
- TF广播器和监听器得到一致的Frame关系。

### 4. 调试工具

```bash
ros2 node list
ros2 topic list
ros2 topic echo /robot_dog/status
ros2 service list
ros2 action list
ros2 param list /parameter_demo
ros2 lifecycle nodes
ros2 run tf2_tools view_frames
ros2 doctor
```

## API和C++能力检查

完成以下解释，不要求死记API：

1. `rclcpp::Node`与Linux进程的关系。
2. `create_publisher<T>()`的模板参数和返回值。
3. `create_subscription<T>()`的消息回调签名。
4. `create_service<T>()`的Request和Response智能指针。
5. `rclcpp_action::create_server<T>()`的Goal、Cancel和Accepted回调。
6. `declare_parameter<T>()`和`get_parameter().as_xxx()`的类型关系。
7. `rclcpp::QoS`链式配置和QoS兼容规则。
8. Lifecycle回调返回`SUCCESS`、`FAILURE`、`ERROR`的区别。
9. TF父Frame、子Frame和时间戳。
10. `shared_ptr`、`unique_ptr`、引用和`const`引用的使用边界。

## 研发流程复盘

### 需求分析

输入是机器人需要通信、配置、启动和坐标转换的功能需求。

### 系统设计

输出是节点、Topic、Service、Action、参数和TF边界。

### 工程实现

使用独立接口包和功能包，CMake声明依赖，Launch负责启动。

### 仿真验证

当前使用模拟状态、模拟步态、模拟QoS和模拟TF，尚未连接真实电机或传感器。

### 集成门禁

所有实验必须能编译、能运行、能用ROS2 CLI观察，并有成功和失败路径测试。

## 自动验收

所有脚本必须串行运行，因为它们共同使用工作空间的`build`和`install`目录：

```bash
./scripts/test_day10.sh
```

覆盖第1至第9天的自动测试。不要并行运行这些脚本，否则多个`colcon build`可能互相覆盖安装文件。

## 第一阶段问题清单

阶段完成不代表机器人功能已经完成。当前仍需在后续阶段解决：

- Service和状态Topic的唯一状态源。
- 真实控制器的固定周期和实时性。
- 硬件驱动错误恢复。
- 多线程Executor和Callback Group设计。
- 传感器时间同步。
- TF树和机器人模型的一致性。

## 实践任务

1. 不看文档画出robot-dog当前通信架构。
2. 给每个节点写出输入、输出、生命周期和故障行为。
3. 解释一次“Topic存在但没有数据”的完整排查流程。
4. 解释一次“Action客户端永久等待”的可能原因。
5. 为第二阶段提出`robot_utils`、`robot_drivers`和`robot_control`的依赖关系。

## 下一步

第十一天开始第二阶段，进入C++工程分层、库设计、硬件抽象和控制节点开发。
