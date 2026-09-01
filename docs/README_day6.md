# 第六天：ROS2参数配置与动态回调

## 今日目标

1. 理解ROS2参数的声明、读取和类型。
2. 使用C++实现启动参数配置。
3. 通过参数回调实现运行时修改。
4. 动态调整节点定时周期并验证参数合法性。

## 背景知识

参数是节点行为的配置接口。机器人名称、控制频率、传感器启用状态等不应硬编码在程序中，也不应每次修改都重新编译。

参数有两个时间点：

```text
节点启动 -> declare_parameter/read parameter
节点运行 -> on_set_parameters_callback
```

启动参数适合确定初始配置，参数回调适合在安全范围内动态调整行为。涉及硬件危险动作的参数仍需要更严格的状态机和权限控制。

## 与Linux嵌入式联系

| ROS2参数 | Linux/嵌入式类比 |
| --- | --- |
| 声明参数 | 驱动初始化默认值 |
| YAML参数文件 | `/etc`配置文件 |
| `ros2 param set` | sysfs/procfs运行时配置 |
| 参数回调 | ioctl参数校验和热更新路径 |

不能只验证参数语法，还要验证参数对运行时资源的影响。例如改变周期时，需要正确重建定时器，不能只更新一个变量。

## 工程实现

### 文件路径

1. `src/robot_dog_day6_parameters/src/parameter_demo.cpp`
   - 声明`robot_name`、`publish_rate_hz`和`enabled`。
   - 校验频率必须为有限正数。
   - 频率变化时重建定时器。
2. `src/robot_dog_day6_parameters/config/parameter_demo.yaml`
   - 保存默认运行配置。
3. `scripts/test_day6.sh`
   - 验证启动参数、合法动态更新和非法更新拒绝。

### 参数设计原因

- `robot_name`是字符串，标识机器人实例。
- `publish_rate_hz`是浮点数，模拟可配置的状态处理频率。
- `enabled`是布尔值，模拟模块启停开关。
- 参数回调先在临时变量中校验全部输入，校验通过后才更新成员，避免半更新状态。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day6_parameters
source install/setup.bash
ros2 run robot_dog_day6_parameters parameter_demo \
  --ros-args \
  --params-file install/robot_dog_day6_parameters/share/robot_dog_day6_parameters/config/parameter_demo.yaml
```

另一个终端检查和修改参数：

```bash
ros2 param list /parameter_demo
ros2 param get /parameter_demo robot_name
ros2 param get /parameter_demo publish_rate_hz
ros2 param set /parameter_demo robot_name field_dog
ros2 param set /parameter_demo publish_rate_hz 5.0
ros2 param set /parameter_demo enabled false
```

非法参数应被拒绝：

```bash
ros2 param set /parameter_demo publish_rate_hz 0.0
```

## 调试方法

```bash
ros2 node list
ros2 node info /parameter_demo
ros2 param list /parameter_demo
ros2 param dump /parameter_demo
ros2 topic echo /parameter_events
ros2 doctor
```

重点观察：

- 参数类型是否匹配。
- 回调返回失败后，旧参数是否保持不变。
- 修改频率后日志周期是否发生变化。
- `enabled=false`时节点是否停止业务输出但仍保持进程运行。

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day6.sh
```

测试覆盖参数文件、参数查询、动态更新和非法频率拒绝。

## 实践任务

1. 增加`max_temperature_celsius`参数，并拒绝超过该阈值的模拟温度。
2. 将`enabled`修改为控制状态Topic发布，而不是只改变日志。
3. 解释参数回调为什么不能执行耗时的电机动作。
4. 设计一个参数分组方案，区分仿真参数、控制参数和硬件参数。

## 研发流程记录

### 输入

- 第五天Launch传递参数的启动流程。
- 运行时调整机器人模块配置的需求。

### 输出

- C++参数节点。
- YAML默认配置。
- 参数类型和数值校验。
- 运行时动态回调和定时器更新。

### 验证方式

- `ros2 param get/set/dump`。
- `ros2 topic echo /parameter_events`。
- `scripts/test_day6.sh`。

## 下一步

第七天学习QoS，分析可靠性、历史深度和持久性如何影响机器人传感器与控制Topic。
