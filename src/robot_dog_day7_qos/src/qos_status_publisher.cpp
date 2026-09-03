#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"

class QosStatusPublisher final : public rclcpp::Node
{
public:
	QosStatusPublisher()
		: Node("qos_status_publisher")
	{
		robot_name_ = this->declare_parameter<std::string>("robot_name", "robot_dog_qos");
		reliability_ = this->declare_parameter<std::string>("reliability", "reliable");
		durability_ = this->declare_parameter<std::string>("durability", "volatile");
		history_ = this->declare_parameter<std::string>("history", "keep_last");
		depth_ = this->declare_parameter<int>("depth", 10);
		publish_period_ms_ = this->declare_parameter<int>("publish_period_ms", 200);
		publish_count_ = this->declare_parameter<int>("publish_count", 0);

		validateParameters();
		const auto qos = createQos();

		rclcpp::PublisherOptions options;
		options.event_callbacks.incompatible_qos_callback =
			[this](rclcpp::QOSOfferedIncompatibleQoSInfo & info)
			{
				RCLCPP_WARN(
					this->get_logger(),
					"QoS不兼容: publisher total_count=%d last_policy_kind=%d",
					info.total_count,
					static_cast<int>(info.last_policy_kind));
			};

		publisher_ = this->create_publisher<robot_dog_interfaces::msg::RobotStatus>(
			"robot_dog/qos_status", qos, options);
		timer_ = this->create_wall_timer(
			std::chrono::milliseconds(publish_period_ms_),
			std::bind(&QosStatusPublisher::timerCallback, this));

		RCLCPP_INFO(
			this->get_logger(),
			"QoS发布者已启动: reliability=%s durability=%s history=%s depth=%d count=%d",
			reliability_.c_str(), durability_.c_str(), history_.c_str(), depth_, publish_count_);
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
		if (depth_ <= 0 || publish_period_ms_ <= 0 || publish_count_ < 0)
		{
			throw std::invalid_argument(
					  "depth and publish_period_ms must be positive; publish_count must be nonnegative");
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

	void timerCallback()
	{
		robot_dog_interfaces::msg::RobotStatus message;
		message.robot_name = robot_name_;
		message.state = "QOS_TEST";
		message.sequence = sequence_++;
		message.battery_percentage = 100.0F;
		message.temperature_celsius = 25.0F;
		publisher_->publish(message);

		RCLCPP_INFO(
			this->get_logger(),
			"发布QoS消息: sequence=%" PRIu64,
			message.sequence);

		++published_messages_;
		if (publish_count_ > 0 && published_messages_ >= publish_count_)
		{
			timer_->cancel();
			RCLCPP_INFO(this->get_logger(), "已达到publish_count，停止发布并保持节点存活");
		}
	}

	rclcpp::Publisher<robot_dog_interfaces::msg::RobotStatus>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
	std::string robot_name_;
	std::string reliability_;
	std::string durability_;
	std::string history_;
	int depth_;
	int publish_period_ms_;
	int publish_count_;
	int published_messages_{0};
	std::uint64_t sequence_{0};
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<QosStatusPublisher>());
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
