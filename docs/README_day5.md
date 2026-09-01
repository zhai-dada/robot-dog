# 第五天：Python Launch文件

## 今日目标

1. 理解Launch文件和直接运行节点的区别。
2. 使用Python描述多个节点的启动配置。
3. 加载参数文件并覆盖Launch参数。
4. 用一个Bringup入口启动robot-dog基础通信系统。

## 背景知识

随着节点数量增加，逐个打开终端并手动输入参数会造成启动顺序、参数和环境不一致。Launch文件相当于机器人系统的启动脚本，把节点、参数、命名空间和输出策略集中描述。

```text
robot_dog_demo.launch.py
    ├── status_publisher
    ├── status_subscriber
    ├── state_server
    ├── gait_action_server
    └── parameter_demo
```

## 与Linux嵌入式联系

| ROS2 Launch | Linux/嵌入式类比 |
| --- | --- |
| `Node` | systemd service或启动脚本中的进程 |
| `DeclareLaunchArgument` | 启动脚本参数或bootargs |
| 参数文件 | `/etc`配置文件、设备树覆盖参数 |
| Bringup | 板级启动流程和系统服务编排 |

Launch负责进程编排，不替代节点内部的业务逻辑。节点仍应能单独启动和单独测试。

## 工程实现

### 文件路径

1. `src/robot_bringup/launch/robot_dog_demo.launch.py`
   - 启动第二至第六天的基础节点。
2. `src/robot_bringup/setup.py`
   - 将Launch文件安装到ROS2包路径。
3. `src/robot_bringup/package.xml`
   - 声明`launch`、`launch_ros`和被启动包依赖。

Launch文件使用`FindPackageShare`查找已安装包的参数文件，不写死`install`绝对路径。这一点很重要，因为真实机器人和仿真机的工作空间路径通常不同。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_bringup
source install/setup.bash
ros2 launch robot_bringup robot_dog_demo.launch.py
```

覆盖Launch参数：

```bash
ros2 launch robot_bringup robot_dog_demo.launch.py \
  robot_name:=field_dog \
  initial_state:=SIT
```

启动后可以检查：

```bash
ros2 node list
ros2 topic list
ros2 service list
ros2 action list
```

## 调试方法

```bash
ros2 launch robot_bringup robot_dog_demo.launch.py --show-args
ros2 node list
ros2 node info /status_publisher
ros2 topic echo /robot_dog/status
ros2 service type /robot_dog/set_state
ros2 action info /robot_dog/execute_gait
ros2 doctor
```

常见问题：

- `Package not found`：没有构建依赖包或没有source工作空间。
- 参数文件找不到：检查目标包是否安装了`config`目录。
- 节点启动但参数未覆盖：检查Launch参数名和节点参数名是否一致。
- Launch退出：查看最先退出的节点日志，Launch通常会连带停止其他进程。

## 测试方法

```bash
cd /home/zhai/robot-dog
./scripts/test_day5.sh
```

测试覆盖Launch包构建、Launch参数解析和五个节点的批量启动。

## 实践任务

1. 增加`use_sim_time`Launch参数并传递给所有节点。
2. 为状态发布节点增加命名空间`robot_dog`。
3. 分析Launch启动多个节点时，哪些依赖由DDS自动发现，哪些依赖由Launch负责。
4. 为真实硬件准备一个单独的`robot_dog_hardware.launch.py`入口。

## 研发流程记录

### 输入

- 已完成的Topic、Service、Action和参数节点。
- 系统需要一条统一启动入口的需求。

### 输出

- Python Bringup包。
- 参数文件加载和Launch参数覆盖。
- 多节点基础系统启动入口。

### 验证方式

- `ros2 launch ... --show-args`。
- `ros2 node list`、`ros2 topic list`、`ros2 service list`和`ros2 action list`。
- `scripts/test_day5.sh`。

## 下一步

第六天学习参数管理，重点是参数声明、类型校验和运行时回调。
