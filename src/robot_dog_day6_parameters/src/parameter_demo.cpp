#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rcl_interfaces/msg/set_parameters_result.hpp"
#include "rclcpp/rclcpp.hpp"

class ParameterDemo final : public rclcpp::Node
{
public:
	ParameterDemo()
		: Node("parameter_demo")
	{
		robot_name_ = this->declare_parameter<std::string>("robot_name", "robot_dog");
		publish_rate_hz_ = this->declare_parameter<double>("publish_rate_hz", 1.0);
		enabled_ = this->declare_parameter<bool>("enabled", true);

		if (!isValidRate(publish_rate_hz_))
		{
			throw std::invalid_argument("publish_rate_hz must be finite and greater than zero");
		}

		parameter_callback_handle_ = this->add_on_set_parameters_callback(
			std::bind(&ParameterDemo::onParameterChange, this, std::placeholders::_1));
		resetTimer();

		RCLCPP_INFO(
			this->get_logger(),
			"参数节点已启动: robot_name=%s publish_rate_hz=%.2f enabled=%s",
			robot_name_.c_str(), publish_rate_hz_, enabled_ ? "true" : "false");
	}

private:
	bool isValidRate(double rate) const
	{
		return std::isfinite(rate) && rate > 0.0;
	}

	rcl_interfaces::msg::SetParametersResult onParameterChange(
		const std::vector<rclcpp::Parameter> & parameters)
	{
		auto result = rcl_interfaces::msg::SetParametersResult();
		result.successful = true;

		std::string next_robot_name = robot_name_;
		double next_publish_rate_hz = publish_rate_hz_;
		bool next_enabled = enabled_;

		for (const auto & parameter : parameters)
		{
			if (parameter.get_name() == "robot_name")
			{
				next_robot_name = parameter.as_string();
			}
			else if (parameter.get_name() == "publish_rate_hz")
			{
				next_publish_rate_hz = parameter.as_double();
			}
			else if (parameter.get_name() == "enabled")
			{
				next_enabled = parameter.as_bool();
			}
		}

		if (!isValidRate(next_publish_rate_hz))
		{
			result.successful = false;
			result.reason = "publish_rate_hz must be finite and greater than zero";
			return result;
		}

		const bool rate_changed = publish_rate_hz_ != next_publish_rate_hz;
		robot_name_ = next_robot_name;
		publish_rate_hz_ = next_publish_rate_hz;
		enabled_ = next_enabled;

		if (rate_changed)
		{
			resetTimer();
		}

		RCLCPP_INFO(
			this->get_logger(),
			"参数已更新: robot_name=%s publish_rate_hz=%.2f enabled=%s",
			robot_name_.c_str(), publish_rate_hz_, enabled_ ? "true" : "false");
		return result;
	}

	void resetTimer()
	{
		const auto period_ms = static_cast<int>(std::max(1.0, 1000.0 / publish_rate_hz_));
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(period_ms),
			std::bind(&ParameterDemo::timerCallback, this));
	}

	void timerCallback()
	{
		if (enabled_)
		{
			RCLCPP_INFO(this->get_logger(), "参数节点运行中: robot_name=%s", robot_name_.c_str());
		}
		else
		{
			RCLCPP_INFO(this->get_logger(), "参数节点已禁用");
		}
	}

	rclcpp::TimerBase::SharedPtr timer_;
	rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_callback_handle_;
	std::string robot_name_;
	double publish_rate_hz_;
	bool enabled_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<ParameterDemo>());
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
