# 第九天：TF坐标变换

## 今日目标

1. 理解机器人为什么需要多个坐标系。
2. 使用C++广播动态TF。
3. 使用`tf2_ros::Buffer`和`TransformListener`查询TF。
4. 使用`ros2 run tf2_tools view_frames`检查TF树。
5. 掌握时间戳、四元数和Frame命名的基本规则。

## 背景知识

机器人不是只有一个坐标系。四足机器人至少需要：

```text
map
 `-- odom
      `-- base_link
           |-- body_link
           |-- imu_link
           `-- lidar_link
```

- `base_link`：机器人底盘参考坐标系。
- `body_link`：机身坐标系。
- `imu_link`：IMU安装坐标系。
- `lidar_link`：激光雷达安装坐标系。

传感器测量值都在自己的坐标系中。导航、控制和融合算法必须知道这些坐标系之间的关系，才能把数据转换到统一参考系。

## 与Linux嵌入式联系

| TF概念 | 嵌入式类比 |
| --- | --- |
| Frame | 设备或传感器的局部参考系 |
| Transform | 驱动层中的安装外参 |
| TF树 | 系统设备拓扑和数据依赖图 |
| 时间戳 | 采样时间、硬件时间戳 |
| TF广播器 | 发布设备外参/动态状态的驱动 |
| TF监听器 | 读取并转换其他模块数据的消费者 |

TF不是简单的全局变量。它是带时间缓存的坐标变换图，查询时必须考虑变换是否存在以及时间是否有效。

## 原理

### 父Frame和子Frame

广播：

```text
parent: base_link
child:  body_link
```

含义是：描述`body_link`相对于`base_link`的位置和姿态。调用：

```cpp
tf_buffer_.lookupTransform("base_link", "body_link", tf2::TimePointZero);
```

返回的是从`body_link`坐标表达转换到`base_link`坐标表达所需的变换。

### 动态TF和静态TF

- 动态TF：随时间变化，例如`odom -> base_link`。
- 静态TF：安装关系不变化，例如`base_link -> lidar_link`。

本日示例使用动态TF，让`body_link`沿小圆轨迹移动，方便观察广播和监听是否正常。真实的传感器安装关系应使用静态TF广播器。

### 时间戳

```cpp
transform.header.stamp = this->get_clock()->now();
```

每个TF都应带时间戳。`get_clock()`返回节点时钟，`now()`返回当前时间。启用仿真时间时，节点时钟会跟随ROS时间，而不是直接使用系统墙上时钟。

### 四元数

三维姿态不能简单用三个角度相加。ROS TF使用四元数表示旋转：

```cpp
rotation.setRPY(roll, pitch, yaw);
transform.transform.rotation = tf2::toMsg(rotation);
```

`setRPY()`把滚转、俯仰、偏航转换成四元数；`toMsg()`把tf2内部类型转换成ROS消息类型。

## 工程实现

### 文件路径

1. `src/robot_dog_day9_tf/src/tf_broadcaster.cpp`
   - 每100ms广播`base_link -> body_link`。
2. `src/robot_dog_day9_tf/src/tf_listener.cpp`
   - 每500ms查询并输出该变换。
3. `scripts/test_day9.sh`
   - 验证广播器和监听器之间的TF链路。

### 依赖

```text
rclcpp
geometry_msgs
tf2
tf2_geometry_msgs
```

## C++与ROS2 API说明

### 1. `std::unique_ptr`

```cpp
std::unique_ptr<tf2_ros::TransformBroadcaster> broadcaster_;
```

`unique_ptr`表示对象只有一个拥有者，离开作用域时自动释放。广播器不需要被多个模块共享，因此使用它比`shared_ptr`更准确。

### 2. `std::make_unique`

```cpp
broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
```

创建对象并返回`unique_ptr`。`*this`把当前Node对象解引用为构造函数需要的Node引用。

### 3. `TransformBroadcaster`

```cpp
broadcaster.sendTransform(transform);
```

作用是向`/tf`发布动态变换。实际代码将它存为成员，确保定时器回调期间对象一直存在。

### 4. `tf2_ros::Buffer`

```cpp
```

Buffer保存收到的TF并提供查询接口。构造时传入节点时钟，保证查询时间和节点使用的ROS时间一致。

### 5. `TransformListener`

```cpp
```

监听`/tf`和`/tf_static`，将收到的变换写入Buffer。Listener必须比查询逻辑存活更久，因此作为成员变量声明在Buffer之后。

### 6. `lookupTransform`

```cpp
  "base_link", "body_link", tf2::TimePointZero);
```

参数：

1. 目标坐标系。
2. 源坐标系。
3. 查询时间。

`tf2::TimePointZero`表示查询Buffer中最新可用的变换。查询失败会抛出`tf2::TransformException`，必须捕获，否则节点会退出。

### 7. `TransformStamped`

```cpp
geometry_msgs::msg::TransformStamped transform;
```

消息包含：

- `header.stamp`：时间戳。
- `header.frame_id`：父Frame。
- `child_frame_id`：子Frame。
- `transform.translation`：平移。
- `transform.rotation`：四元数旋转。

### 8. `create_wall_timer`

```cpp
timer_ = this->create_wall_timer(
  std::chrono::milliseconds(100),
  std::bind(&TfBroadcaster::broadcastTransform, this));
```

第一参数是周期，第二参数是回调。它使用节点Executor调度，和第二天定时发布一样，不是硬实时定时器。

## 编译运行

```bash
cd /home/zhai/robot-dog
source /opt/ros/humble/setup.bash
colcon build --packages-up-to robot_dog_day9_tf
source install/setup.bash
```

终端一：

```bash
ros2 run robot_dog_day9_tf tf_broadcaster
```

终端二：

```bash
ros2 run robot_dog_day9_tf tf_listener
```

终端三检查TF：

```bash
ros2 run tf2_tools view_frames
ros2 run tf2_ros tf2_echo base_link body_link
```

## 调试方法

```bash
ros2 node list
ros2 topic list
ros2 topic echo /tf
ros2 topic echo /tf_static
ros2 run tf2_tools view_frames
ros2 run tf2_ros tf2_echo base_link body_link
ros2 doctor
```

排查顺序：

1. 广播器是否运行。
2. `/tf`是否出现。
3. 父Frame和子Frame名称是否完全一致。
4. 时间戳是否来自正确的ROS时钟。
5. Listener是否在广播后启动并等待Buffer填充。

## 测试方法

```bash
./scripts/test_day9.sh
```

测试验证动态广播节点启动、监听器成功查询变换以及指定Frame关系存在。

## 实践任务

1. 增加`base_link -> imu_link`静态变换。
2. 使用`tf2_ros::StaticTransformBroadcaster`比较静态广播和动态广播。
3. 让Listener查询一个不存在的Frame，观察异常信息。
4. 使用`view_frames`检查TF树是否存在环路或孤立Frame。
5. 解释为什么传感器驱动必须使用正确的硬件时间戳。

## 下一步

第十天进行第一阶段综合验收，回顾Node、Topic、Service、Action、参数、QoS、Lifecycle和TF的分工。
