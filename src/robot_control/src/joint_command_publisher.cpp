#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "robot_utils/angle_utils.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

class JointCommandPublisher final : public rclcpp::Node
{
public:
	JointCommandPublisher()
		: Node("joint_command_publisher"), phase_(0.0)
	{
		publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>(
			"robot_dog/joint_commands", rclcpp::QoS(10));
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(100),
			std::bind(&JointCommandPublisher::publishCommand, this));

		RCLCPP_INFO(this->get_logger(), "关节控制节点已启动");
	}

private:
	void publishCommand()
	{
		std_msgs::msg::Float64MultiArray message;
		message.data = {
			robot_utils::AngleUtils::clamp(std::sin(phase_), -1.57, 1.57),
			robot_utils::AngleUtils::clamp(std::sin(phase_ + 2.09), -1.57, 1.57),
			robot_utils::AngleUtils::clamp(std::sin(phase_ + 4.18), -1.57, 1.57)};
		publisher_->publish(message);
		phase_ += 0.1;
	}

	rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
	double phase_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<JointCommandPublisher>());
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
