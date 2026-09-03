#include <cinttypes>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"

class QosStatusSubscriber final : public rclcpp::Node
{
public:
	QosStatusSubscriber()
		: Node("qos_status_subscriber")
	{
		reliability_ = this->declare_parameter<std::string>("reliability", "reliable");
		durability_ = this->declare_parameter<std::string>("durability", "volatile");
		history_ = this->declare_parameter<std::string>("history", "keep_last");
		depth_ = this->declare_parameter<int>("depth", 10);

		validateParameters();
		const auto qos = createQos();

		rclcpp::SubscriptionOptions options;
		options.event_callbacks.incompatible_qos_callback =
			[this](rclcpp::QOSRequestedIncompatibleQoSInfo & info)
			{
				RCLCPP_WARN(
					this->get_logger(),
					"QoS不兼容: subscriber total_count=%d last_policy_kind=%d",
					info.total_count,
					static_cast<int>(info.last_policy_kind));
			};

		subscription_ = this->create_subscription<robot_dog_interfaces::msg::RobotStatus>(
			"robot_dog/qos_status",
			qos,
			std::bind(&QosStatusSubscriber::statusCallback, this, std::placeholders::_1),
			options);

		RCLCPP_INFO(
			this->get_logger(),
			"QoS订阅者已启动: reliability=%s durability=%s history=%s depth=%d",
			reliability_.c_str(), durability_.c_str(), history_.c_str(), depth_);
	}

private:
	void validateParameters() const
	{
		if (reliability_ != "reliable" && reliability_ != "best_effort")
		{
			throw std::invalid_argument("reliability must be reliable or best_effort");
		}
		if (durability_ != "volatile" && durability_ != "transient_local")
		{
			throw std::invalid_argument("durability must be volatile or transient_local");
		}
		if (history_ != "keep_last" && history_ != "keep_all")
		{
			throw std::invalid_argument("history must be keep_last or keep_all");
		}
		if (depth_ <= 0)
		{
			throw std::invalid_argument("depth must be positive");
		}
	}

	rclcpp::QoS createQos() const
	{
		rclcpp::QoS qos = history_ == "keep_all" ?
			rclcpp::QoS(rclcpp::KeepAll()) : rclcpp::QoS(rclcpp::KeepLast(depth_));

		if (reliability_ == "reliable")
		{
			qos.reliable();
		}
		else
		{
			qos.best_effort();
		}

		if (durability_ == "transient_local")
		{
			qos.transient_local();
		}
		else
		{
			qos.durability_volatile();
		}

		return qos;
	}

	void statusCallback(
		const robot_dog_interfaces::msg::RobotStatus::ConstSharedPtr message)
	{
		RCLCPP_INFO(
			this->get_logger(),
			"接收QoS消息: robot_name=%s sequence=%" PRIu64,
			message->robot_name.c_str(),
			message->sequence);
	}

	rclcpp::Subscription<robot_dog_interfaces::msg::RobotStatus>::SharedPtr subscription_;
	std::string reliability_;
	std::string durability_;
	std::string history_;
	int depth_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<QosStatusSubscriber>());
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
