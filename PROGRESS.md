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
- [ ] 服务端和客户端
- [ ] 自定义服务类型
- [ ] 服务调用

### 第4天：动作通信
- [ ] 动作客户端
- [ ] 动作服务端
- [ ] 长期任务处理

### 第5天：Launch文件
- [ ] Python Launch
- [ ] XML Launch
- [ ] 批量启动节点

### 第6天：参数配置
- [ ] 参数声明
- [ ] 参数获取
- [ ] 参数回调

### 第7天：QoS策略
- [ ] 可靠性策略
- [ ] 持久性策略
- [ ] 历史策略

### 第8天：Lifecycle节点
- [ ] 生命周期管理
- [ ] 状态机
- [ ] 配置管理

### 第9天：TF变换
- [ ] 坐标变换
- [ ] TF广播
- [ ] TF监听

### 第10天：第一阶段总结
- [ ] 综合练习
- [ ] 知识梳理
- [ ] 准备第二阶段

## 项目结构

```
robot-dog/
├── src/
│   ├── robot_dog_basics/          # Python节点包
│   ├── robot_dog_basics_cpp/      # C++节点包
│   ├── robot_dog_day1_exercises/  # 第一天C++练习包
│   ├── robot_dog_interfaces/      # ROS2消息接口包
│   └── robot_dog_day2_topics/     # 第二天Topic示例
├── docs/
│   ├── README_day1.md
│   └── README_day2.md
├── scripts/
│   ├── test_day1.sh
│   └── test_day2.sh
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
