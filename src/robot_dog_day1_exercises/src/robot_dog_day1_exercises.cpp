#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

class CountdownNode : public rclcpp::Node
{
public:
	CountdownNode()
		: Node("counter_node"), count_(100), message_("机器人狗系统工程")
	{
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(500),
			std::bind(&CountdownNode::timerCallback, this));

		RCLCPP_INFO(
			this->get_logger(), "倒计时节点已启动: %s", message_.c_str());
	}

private:
	void timerCallback()
	{
		RCLCPP_INFO(this->get_logger(), "计数: %d", count_);

		if (count_ == 0)
		{
			timer_->cancel();
			rclcpp::shutdown();
			return;
		}

		--count_;
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
		auto node = std::make_shared<CountdownNode>();
		rclcpp::spin(node);
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
