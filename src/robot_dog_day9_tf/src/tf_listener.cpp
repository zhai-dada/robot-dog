#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "tf2/exceptions.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

class TfListener final : public rclcpp::Node
{
public:
	TfListener()
		: Node("tf_listener"), tf_buffer_(this->get_clock()), tf_listener_(tf_buffer_)
	{
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(500),
			std::bind(&TfListener::lookupTransform, this));

		RCLCPP_INFO(this->get_logger(), "TF监听节点已启动: 查询base_link -> body_link");
	}

private:
	void lookupTransform()
	{
		try
		{
			const auto transform = tf_buffer_.lookupTransform(
				"base_link", "body_link", tf2::TimePointZero);

			RCLCPP_INFO(
				this->get_logger(),
				"TF: x=%.3f y=%.3f z=%.3f",
				transform.transform.translation.x,
				transform.transform.translation.y,
				transform.transform.translation.z);
		}
		catch (const tf2::TransformException & exception)
		{
			RCLCPP_WARN(this->get_logger(), "TF查询失败: %s", exception.what());
		}
	}

	tf2_ros::Buffer tf_buffer_;
	tf2_ros::TransformListener tf_listener_;
	rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<TfListener>());
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
