#!/usr/bin/env python3
"""
A minimal ROS 2 node example.

This module demonstrates the basic structure of a timer-driven node.
"""

import rclpy
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node


class HelloWorldNode(Node):
    """A minimal timer-driven ROS 2 node."""

    def __init__(self):
        """Initialize the node."""
        # 调用父类构造函数，创建名为'my_first_node'的节点
        super().__init__('my_first_node')

        # 创建定时器，每2秒执行一次回调函数
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
    # 初始化ROS2
    rclpy.init(args=args)

    # 创建节点
    node = HelloWorldNode()

    try:
        # 运行节点
        rclpy.spin(node)
    except (KeyboardInterrupt, ExternalShutdownException):
        # ROS 2 may already have invalidated the context when a signal arrives.
        pass
    finally:
        # 清理资源
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
