#include <cinttypes>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"

class StatusSubscriber final : public rclcpp::Node
{
public:
	StatusSubscriber() : Node("status_subscriber")
	{
		subscription_ = this->create_subscription<robot_dog_interfaces::msg::RobotStatus>(
			"robot_dog/status",
			rclcpp::QoS(10),
			std::bind(
				&StatusSubscriber::statusCallback,
				this,
				std::placeholders::_1));

		RCLCPP_INFO(this->get_logger(), "状态订阅节点已启动");
	}

private:
	void statusCallback(const robot_dog_interfaces::msg::RobotStatus::ConstSharedPtr message)
	{
		RCLCPP_INFO(
			this->get_logger(),
			"接收: robot_name=%s state=%s sequence=%" PRIu64 " battery=%.1f%% temperature=%.2f °C",
			message->robot_name.c_str(), message->state.c_str(), message->sequence,
			message->battery_percentage, message->temperature_celsius);
	}

	rclcpp::Subscription<robot_dog_interfaces::msg::RobotStatus>::SharedPtr subscription_;
};

int main(int argc, char *argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<StatusSubscriber>());
	}
	catch (const std::exception &e)
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
