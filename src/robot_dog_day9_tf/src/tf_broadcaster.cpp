#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/transform_broadcaster.h"

class TfBroadcaster final : public rclcpp::Node
{
public:
	TfBroadcaster()
		: Node("tf_broadcaster"), sequence_(0)
	{
		broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(100),
			std::bind(&TfBroadcaster::broadcastTransform, this));

		RCLCPP_INFO(this->get_logger(), "TF广播节点已启动: base_link -> body_link");
	}

private:
	void broadcastTransform()
	{
		geometry_msgs::msg::TransformStamped transform;
		transform.header.stamp = this->get_clock()->now();
		transform.header.frame_id = "base_link";
		transform.child_frame_id = "body_link";

		const double phase = static_cast<double>(sequence_) * 0.05;
		transform.transform.translation.x = 0.10 * std::cos(phase);
		transform.transform.translation.y = 0.10 * std::sin(phase);
		transform.transform.translation.z = 0.35;

		tf2::Quaternion rotation;
		rotation.setRPY(0.0, 0.0, 0.10 * std::sin(phase));
		transform.transform.rotation = tf2::toMsg(rotation);

		broadcaster_->sendTransform(transform);
		++sequence_;
	}

	std::unique_ptr<tf2_ros::TransformBroadcaster> broadcaster_;
	rclcpp::TimerBase::SharedPtr timer_;
	std::uint64_t sequence_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<TfBroadcaster>());
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
