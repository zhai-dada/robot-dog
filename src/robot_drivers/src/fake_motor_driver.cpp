#include <functional>
#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "robot_utils/angle_utils.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

class FakeMotorDriver final : public rclcpp::Node
{
public:
	FakeMotorDriver()
		: Node("fake_motor_driver")
	{
		subscription_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
			"robot_dog/joint_commands",
			rclcpp::QoS(10),
			std::bind(&FakeMotorDriver::commandCallback, this, std::placeholders::_1));

		RCLCPP_INFO(this->get_logger(), "模拟电机驱动已启动，等待关节目标");
	}

private:
	void commandCallback(const std_msgs::msg::Float64MultiArray::ConstSharedPtr message)
	{
		if (message->data.size() != 3U)
		{
			RCLCPP_WARN(this->get_logger(), "忽略错误关节目标，期望3个关节值");
			return;
		}

		std::vector<double> safe_commands;
		safe_commands.reserve(message->data.size());
		for (const double command : message->data)
		{
			safe_commands.push_back(robot_utils::AngleUtils::clamp(command, -1.57, 1.57));
		}

		RCLCPP_INFO(
			this->get_logger(),
			"模拟电机接收目标: [%.3f, %.3f, %.3f]",
			safe_commands[0], safe_commands[1], safe_commands[2]);
	}

	rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<FakeMotorDriver>());
	}
	catch (const std::exception & e)
	{
		RCLCPP_ERROR(rclcpp::get_logger("main"), "节点异常: %s", e.what());
		return_code = 1;
	}

	if (rclcpp::ok())
	{
		rclcpp::shutdown();
	}

	return return_code;
}
