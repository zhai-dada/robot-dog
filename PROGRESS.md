# 机器人狗系统工程 - 学习进度

## 第一阶段：ROS2基础

### 第1天：ROS2基础 - 第一个节点
- ✅ 创建ROS2工作空间
- ✅ 编写Python节点
- ✅ 编写C++节点
- ✅ 编译和测试
- ✅ 完成C++练习：节点修改与倒计时
- 📝 详细文档：docs/README_day1.md

### 第2天：话题通信
- [x] 发布者和订阅者
- [x] 自定义消息类型
- [x] 多节点通信
- [x] 参数配置
- 📝 详细文档：docs/README_day2.md

### 第3天：服务通信
- [x] 服务端和客户端
- [x] 自定义服务类型
- [x] 服务调用
- 📝 详细文档：docs/README_day3.md

### 第4天：动作通信
- [x] 动作客户端
- [x] 动作服务端
- [x] 长期任务处理
- 📝 详细文档：docs/README_day4.md

### 第5天：Launch文件
- [x] Python Launch
- [ ] XML Launch
- [x] 批量启动节点
- 📝 详细文档：docs/README_day5.md

### 第6天：参数配置
- [x] 参数声明
- [x] 参数获取
- [x] 参数回调
- 📝 详细文档：docs/README_day6.md

### 第7天：QoS策略
- [x] 可靠性策略
- [x] 持久性策略
- [x] 历史策略
- 📝 详细文档：docs/README_day7.md

### 第8天：Lifecycle节点
- [x] 生命周期管理
- [x] 状态机
- [x] 配置管理
- 📝 详细文档：docs/README_day8.md

### 第9天：TF变换
- [x] 坐标变换
- [x] TF广播
- [x] TF监听
- 📝 详细文档：docs/README_day9.md

### 第10天：第一阶段总结
- [x] 综合练习
- [x] 知识梳理
- [x] 准备第二阶段
- 📝 详细文档：docs/README_day10.md

## 第二阶段：ROS2 C++开发

### 第11天：C++工程分层
- [x] robot_utils工具库
- [x] robot_drivers模拟驱动
- [x] robot_control控制节点
- 📝 详细文档：docs/README_day11.md

## 项目结构

```
robot-dog/
├── src/
│   ├── robot_dog_basics/          # Python节点包
│   ├── robot_dog_basics_cpp/      # C++节点包
│   ├── robot_dog_day1_exercises/  # 第一天C++练习包
│   ├── robot_dog_interfaces/      # ROS2消息接口包
│   ├── robot_dog_day2_topics/     # 第二天Topic示例
│   ├── robot_dog_day3_services/   # 第三天Service示例
│   ├── robot_dog_day4_actions/    # 第四天Action示例
│   ├── robot_bringup/              # 系统Launch入口
│   ├── robot_dog_day6_parameters/ # 第六天参数示例
│   ├── robot_dog_day7_qos/        # 第七天QoS实验
│   ├── robot_dog_day8_lifecycle/  # 第八天Lifecycle实验
│   ├── robot_dog_day9_tf/         # 第九天TF实验
│   ├── robot_utils/               # 通用C++工具库
│   ├── robot_drivers/             # 驱动抽象示例
│   ├── robot_control/             # 控制层示例
├── docs/
│   ├── README_day1.md
│   ├── README_day2.md
│   ├── README_day3.md
│   ├── README_day4.md
│   ├── README_day5.md
│   ├── README_day6.md
│   ├── README_day7.md
│   ├── README_day8.md
│   ├── README_day9.md
│   ├── README_day10.md
│   └── README_day11.md
├── scripts/
│   ├── test_day1.sh
│   ├── test_day2.sh
│   ├── test_day3.sh
│   ├── test_day4.sh
│   ├── test_day5.sh
│   ├── test_day6.sh
│   ├── test_day7.sh
│   ├── test_day8.sh
│   ├── test_day9.sh
│   ├── test_day10.sh
│   └── test_day11.sh
├── install/                       # 编译输出
├── build/                         # 编译中间文件
├── log/                           # 编译日志
├── README.md                      # 项目入口
└── PROGRESS.md                    # 学习进度
```

## 学习资源

1. ROS2官方文档: https://docs.ros.org/en/humble/Tutorials.html
2. ROS2示例: https://github.com/ros2/examples
3. ROS2机器人开发: https://github.com/AcademySoftwareFoundation/robot-dog
