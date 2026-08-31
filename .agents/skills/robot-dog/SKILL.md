---
name: robot-dog
description: 机器人狗系统工程技能，包含学习路线、项目结构、编程规范、调试方法等详细内容。
license: Apache-2.0
---

# 机器人狗系统工程技能

# 工程项目

所有学习围绕：robot-dog

项目结构：

robot-dog/                           # 工作空间根目录（可带短横线，Git 仓库根）
├── src/                             # ROS 2 功能包源码（必须全小写+下划线）
│   ├── robot_description/           # 机器人模型（URDF/Xacro/Meshes）
│   │   ├── urdf/
│   │   ├── meshes/
│   │   └── config/
│   ├── robot_interfaces/            # 自定义消息/服务/动作（仅含 .msg/.srv/.action）
│   ├── robot_bringup/               # Launch 文件与运行时参数配置
│   │   ├── launch/
│   │   └── config/
│   ├── robot_utils/                 # 通用工具库（数学、日志、数据结构）
│   ├── robot_drivers/               # 传感器/电机串口或 I2C 驱动（硬件抽象层）
│   ├── robot_control/               # 运动学、步态、状态机（依赖 drivers + interfaces）
│   ├── robot_perception/            # 视觉、激光雷达处理、多传感器融合
│   ├── robot_navigation/            # Nav2 导航配置与适配
│   ├── robot_slam/                  # SLAM 算法配置与启动
│   └── robot_simulation/            # Gazebo 仿真世界与模型
│
├── build/                           # 编译中间文件（加入 .gitignore）
├── install/                         # 编译产物（加入 .gitignore）
├── log/                             # 编译与运行日志（加入 .gitignore）
│
├── docs/                            # 项目文档（架构、设计、学习笔记）
│   ├── architecture.md
│   ├── design.md
│   └── learning.md
├── scripts/                         # 自动化脚本（环境配置、数据预处理、CI 辅助）
├── AGENTS.md                        # AI/LLM 代理行为规范（保留，不提交个人缓存）
├── .gitignore                       # 必须创建，忽略 build/install/log/.opencode/ 等
├── README.md                        # 项目介绍与快速开始
└── LICENSE                          # 开源协议（如 Apache-2.0）

所有新知识尽量加入该工程。

---

# 编程语言规范

## C++

机器人核心代码优先使用：

C++17

用于：

- ROS2核心节点
- 控制器
- 运动学
- 硬件接口
- 实时性要求高模块

重点学习：

- 类
- 面向对象
- 模板
- STL
- 智能指针
- Lambda
- CMake
- rclcpp

---

## Python

Python用于：

- Launch文件
- 自动化脚本
- 配置工具
- 数据处理
- 测试程序
- 简单辅助节点

原则：

核心机器人能力使用C++。
辅助工具使用Python。

---

# 学习路线

# 第一阶段 ROS2基础

目标：
理解ROS2通信模型。

学习：

- ROS2架构
- Node
- Topic
- Service
- Action
- Parameter
- Launch

实践：
创建：

Python节点
C++节点

掌握：

colcon build
ros2 run
ros2 launch
ros2 topic
ros2 node

---

# 第二阶段 ROS2 C++开发

目标：

能够开发机器人软件。

学习：

- ROS2 C++ API
- rclcpp
- package.xml
- CMake
- QoS
- Lifecycle Node

实践：

实现：
motor_node
sensor_node
controller_node

---

# 第三阶段机器人建模

目标：

建立自己的四足机器人模型。

学习：
URDF
Xacro
TF

掌握：

- link
- joint
- inertial
- collision
- visual

---

# 第四阶段机器人仿真

工具：

- Gazebo
- RViz2

学习：

- robot_state_publisher
- ros2_control
- controller

目标：

机器人可以：

- 加载模型
- 显示TF
- 接收控制命令
- 模拟运动

---

# 第五阶段机器人传感器

学习：

## IMU

理解：

sensor_msgs/Imu

## LiDAR

理解：

sensor_msgs/LaserScan

## Camera

理解：

sensor_msgs/Image

目标：

机器人具备环境感知能力。

---

# 第六阶段SLAM

学习：

slam_toolbox

理解：

- 地图建立
- 定位
- 坐标变换


目标：
机器人能够：
扫描环境
生成地图。

---

# 第七阶段自主导航

学习：

Nav2

理解：

- Costmap
- Planner
- Controller
- Behavior Tree


目标：
机器人：
输入目标点
自动规划路线
避障移动。

---

# 第八阶段四足机器人算法

学习：

机器人数学基础：

- 向量
- 矩阵
- 坐标变换
- 四元数

运动学：

- 正运动学
- 逆运动学
- Jacobian

控制：

- PID
- 状态估计
- 卡尔曼滤波

高级：

- MPC
- Whole Body Control

步态：

- Crawl
- Walk
- Trot
- Pace

---

# 机器人研发流程

所有功能按照真实工程流程：

需求分析
↓
系统设计
↓
软件架构
↓
机器人建模
↓
仿真
↓
算法开发
↓
系统集成
↓
硬件迁移

每个阶段说明：

- 输入
- 输出
- 验证方式

---

# 代码生成规范

生成代码时必须说明：

1.文件路径
2.文件作用
3.代码设计原因
4.依赖
5.编译方法
6.运行方法
7.测试方法

禁止：

一次生成大型未知代码。

必须：

先设计
↓
解释
↓
实现
↓
测试

---

# 调试规范


遇到问题：

不要直接修改代码。

按照顺序：

## ROS节点

ros2 node list

## Topic

ros2 topic list

## 数据

ros2 topic echo

## TF

ros2 run tf2_tools view_frames

## 系统检查

ros2 doctor

## 编译问题

检查：

- package.xml
- CMakeLists.txt
- dependency
- workspace

---

# 最终毕业目标

完成：

robot-dog


虚拟四足机器人系统。

必须实现：


1.Gazebo加载四足机器人
2.RViz显示机器人状态
3.ROS2控制机器人
4.模拟LiDAR
5.模拟IMU
6.完成SLAM建图
7.完成Nav2导航
8.实现机器人运动学
9.实现基础步态控制
10.形成完整ROS2工程架构

最终：

能够将仿真系统迁移到真实机器人平台。