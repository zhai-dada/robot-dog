# robot-dog系统架构

## 当前分层

```text
robot_bringup
    |
    +-- robot_control
    +-- robot_drivers
    +-- robot_dog_day2_topics
    +-- robot_dog_day3_services
    +-- robot_dog_day4_actions
    +-- robot_dog_day8_lifecycle
    `-- robot_dog_day9_tf

robot_dog_interfaces  <- 共享消息、服务和动作
robot_utils           <- 通用C++工具库
```

## 目标分层

```text
robot_ai
robot_navigation / robot_slam
robot_perception
robot_control
robot_drivers
robot_utils
robot_description / robot_simulation
robot_dog_interfaces
```

## 原则

- 接口包不包含业务节点。
- 控制层不直接访问硬件设备文件。
- 驱动层隐藏串口、CAN、I2C和SPI细节。
- Launch负责编排，节点负责业务。
- 仿真接口和真实硬件接口保持一致。
