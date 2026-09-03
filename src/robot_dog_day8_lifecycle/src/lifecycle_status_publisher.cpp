#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "rclcpp_lifecycle/node_interfaces/lifecycle_node_interface.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"

class LifecycleStatusPublisher final : public rclcpp_lifecycle::LifecycleNode
{
public:
	using CallbackReturn =
		rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

	LifecycleStatusPublisher()
		: rclcpp_lifecycle::LifecycleNode("lifecycle_status_publisher")
	{
		this->declare_parameter<std::string>("robot_name", "lifecycle_robot_dog");
		this->declare_parameter<int>("publish_period_ms", 500);
		this->declare_parameter<double>("battery_percentage", 90.0);
		this->declare_parameter<bool>("fail_configuration", false);

		RCLCPP_INFO(this->get_logger(), "Lifecycle节点已创建，当前状态为unconfigured");
	}

	CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_INFO(
			this->get_logger(),
			"on_configure: previous_state=%s",
			previous_state.label().c_str());

		try
		{
			robot_name_ = this->get_parameter("robot_name").as_string();
			publish_period_ms_ = this->get_parameter("publish_period_ms").as_int();
			battery_percentage_ = this->get_parameter("battery_percentage").as_double();
			const bool fail_configuration = this->get_parameter("fail_configuration").as_bool();

			if (fail_configuration)
			{
				RCLCPP_ERROR(this->get_logger(), "参数要求模拟configure失败");
				return CallbackReturn::FAILURE;
			}
			if (publish_period_ms_ <= 0)
			{
				RCLCPP_ERROR(this->get_logger(), "publish_period_ms必须大于0");
				return CallbackReturn::FAILURE;
			}
			if (battery_percentage_ < 0.0 || battery_percentage_ > 100.0)
			{
				RCLCPP_ERROR(this->get_logger(), "battery_percentage必须位于[0, 100]");
				return CallbackReturn::FAILURE;
			}

			publisher_ = this->create_publisher<robot_dog_interfaces::msg::RobotStatus>(
				"robot_dog/lifecycle_status",
				rclcpp::QoS(10));
			timer_ = this->create_wall_timer(
				std::chrono::milliseconds(publish_period_ms_),
				std::bind(&LifecycleStatusPublisher::timerCallback, this));
			timer_->cancel();
			sequence_ = 0;

			RCLCPP_INFO(
				this->get_logger(),
				"资源配置完成: robot_name=%s period_ms=%" PRId64 " battery=%.1f%%",
				robot_name_.c_str(), publish_period_ms_, battery_percentage_);
			return CallbackReturn::SUCCESS;
		}
		catch (const std::exception & e)
		{
			RCLCPP_ERROR(this->get_logger(), "configure异常: %s", e.what());
			releaseResources();
			return CallbackReturn::ERROR;
		}
	}

	CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_INFO(
			this->get_logger(),
			"on_activate: previous_state=%s",
			previous_state.label().c_str());

		if (!publisher_ || !timer_)
		{
			RCLCPP_ERROR(this->get_logger(), "资源尚未配置，无法activate");
			return CallbackReturn::FAILURE;
		}

		publisher_->on_activate();
		timer_->reset();
		RCLCPP_INFO(this->get_logger(), "Lifecycle节点已激活，开始发布状态");
		return CallbackReturn::SUCCESS;
	}

	CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_INFO(
			this->get_logger(),
			"on_deactivate: previous_state=%s",
			previous_state.label().c_str());

		if (timer_)
		{
			timer_->cancel();
		}
		if (publisher_)
		{
			publisher_->on_deactivate();
		}

		RCLCPP_INFO(this->get_logger(), "Lifecycle节点已停用，停止发布但保留资源");
		return CallbackReturn::SUCCESS;
	}

	CallbackReturn on_cleanup(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_INFO(
			this->get_logger(),
			"on_cleanup: previous_state=%s",
			previous_state.label().c_str());
		releaseResources();
		RCLCPP_INFO(this->get_logger(), "Lifecycle资源已释放");
		return CallbackReturn::SUCCESS;
	}

	CallbackReturn on_shutdown(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_INFO(
			this->get_logger(),
			"on_shutdown: previous_state=%s",
			previous_state.label().c_str());
		releaseResources();
		return CallbackReturn::SUCCESS;
	}

	CallbackReturn on_error(const rclcpp_lifecycle::State & previous_state) override
	{
		RCLCPP_ERROR(
			this->get_logger(),
			"on_error: previous_state=%s",
			previous_state.label().c_str());
		releaseResources();
		return CallbackReturn::SUCCESS;
	}

private:
	void timerCallback()
	{
		robot_dog_interfaces::msg::RobotStatus message;
		message.robot_name = robot_name_;
		message.state = "ACTIVE";
		message.sequence = sequence_++;
		message.battery_percentage = static_cast<float>(battery_percentage_);
		message.temperature_celsius = 25.0F;
		publisher_->publish(message);

		RCLCPP_INFO(
			this->get_logger(),
			"发布Lifecycle状态: sequence=%" PRIu64,
			message.sequence);
	}

	void releaseResources()
	{
		if (timer_)
		{
			timer_->cancel();
		}
		timer_.reset();
		publisher_.reset();
		sequence_ = 0;
	}

	rclcpp_lifecycle::LifecyclePublisher<
		robot_dog_interfaces::msg::RobotStatus>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
	std::string robot_name_;
	std::int64_t publish_period_ms_{0};
	double battery_percentage_{0.0};
	std::uint64_t sequence_{0};
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		auto node = std::make_shared<LifecycleStatusPublisher>();
		rclcpp::spin(node->get_node_base_interface());
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
