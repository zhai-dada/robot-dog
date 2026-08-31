/**
 * 第一个ROS2 C++节点 - Hello World
 * 文件路径: src/robot_dog_basics_cpp/src/hello_world_node.cpp
 * 作用: 演示最基本的ROS2 C++节点结构
 */

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
			std::bind(&HelloWorldNode::timerCallback, this));

		RCLCPP_INFO(
			this->get_logger(), "Hello World C++节点已启动! %s", message_.c_str());
	}

private:
	void timerCallback()
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

	try
	{
		rclcpp::spin(std::make_shared<HelloWorldNode>());
	}
	catch (const std::exception & e)
	{
		RCLCPP_ERROR(rclcpp::get_logger("main"), "节点异常: %s", e.what());
	}

	if (rclcpp::ok())
	{
		rclcpp::shutdown();
	}

	return 0;
}
