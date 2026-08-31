#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"

class StatusPublisher final : public rclcpp::Node
{
public:
	StatusPublisher() : Node("status_publisher"), sequence_(0)
	{
		robot_name_ = this->declare_parameter<std::string>("robot_name", "robot_dog");
		state_ = this->declare_parameter<std::string>("state", "STANDING");
		publish_period_ms_ = this->declare_parameter<int>("publish_period_ms", 500);
		battery_percentage_ = this->declare_parameter<double>("battery_percentage", 100.0);
		temperature_celsius_ = this->declare_parameter<std::float_t>("temperature_celsius_", 25.0);

		if (publish_period_ms_ <= 0)
		{
			throw std::invalid_argument("publish_period_ms must be greater than zero");
		}

		if(temperature_celsius_ <= -273.15)
		{
			throw std::invalid_argument("temperature_celsius_ must be greater than -273.15");
		}

		if (battery_percentage_ < 0.0 || battery_percentage_ > 100.0)
		{
			throw std::invalid_argument("battery_percentage must be in the range [0, 100]");
		}

		publisher_ = this->create_publisher<robot_dog_interfaces::msg::RobotStatus>("robot_dog/status", rclcpp::QoS(10));

		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(publish_period_ms_),
			std::bind(&StatusPublisher::publishStatus, this));

		RCLCPP_INFO(
			this->get_logger(),
			"状态发布节点已启动: robot_name=%s state=%s period_ms=%d",
			robot_name_.c_str(), state_.c_str(), publish_period_ms_);
	}

private:
	void publishStatus()
	{
		robot_dog_interfaces::msg::RobotStatus message;

		message.robot_name = robot_name_;
		message.state = state_;
		message.sequence = sequence_++;
		message.battery_percentage = static_cast<float>(battery_percentage_);
		message.temperature_celsius = static_cast<float>(temperature_celsius_);

		publisher_->publish(message);

		RCLCPP_INFO(
			this->get_logger(),
			"发布: robot_name=%s state=%s sequence=%" PRIu64 " battery=%.1f%% temperature=%.1f%% °C", 
			message.robot_name.c_str(), message.state.c_str(), message.sequence,
			message.battery_percentage, message.temperature_celsius);
	}

	rclcpp::Publisher<robot_dog_interfaces::msg::RobotStatus>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
	std::string robot_name_;
	std::string state_;
	int publish_period_ms_;
	double battery_percentage_;
	std::uint64_t sequence_;
	std::float_t temperature_celsius_;
};

int main(int argc, char *argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<StatusPublisher>());
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
