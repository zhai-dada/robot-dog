#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/srv/set_robot_state.hpp"
#include "robot_dog_interfaces/srv/get_robot_state.hpp"

class StateClient final : public rclcpp::Node
{
public:
	StateClient() : Node("state_client")
	{
		target_state_ = this->declare_parameter<std::string>("target_state", "TROT");
		timeout_ms_ = this->declare_parameter<int>("timeout_ms", 3000);
		status_flag_ = this->declare_parameter<bool>("status_flag", 1);

		if (timeout_ms_ <= 0)
		{
			throw std::invalid_argument("timeout_ms must be greater than zero");
		}

		if (status_flag_ == 1)
		{
			client_set_ = this->create_client<robot_dog_interfaces::srv::SetRobotState>(
				"robot_dog/set_state");
		}
		else
		{
			client_get_ = this->create_client<robot_dog_interfaces::srv::GetRobotState>(
				"robot_dog/get_state");
		}
	}

	int sendRequest()
	{
		if (status_flag_ == 1)
		{
			auto client_ = client_set_;
			if (!client_->wait_for_service(std::chrono::milliseconds(timeout_ms_)))
			{
				RCLCPP_ERROR(this->get_logger(), "服务不可用: /robot_dog/set_state");
				return 1;
			}

			auto request = std::make_shared<robot_dog_interfaces::srv::SetRobotState::Request>();
			request->target_state = target_state_;

			RCLCPP_INFO(this->get_logger(), "请求状态切换: target_state=%s", target_state_.c_str());

			auto future = client_->async_send_request(request);

			const auto result = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), future,
				std::chrono::milliseconds(timeout_ms_));

			if (result != rclcpp::FutureReturnCode::SUCCESS)
			{
				RCLCPP_ERROR(this->get_logger(), "等待服务响应超时或失败");
				return 1;
			}

			const auto response = future.get();
			if (response->success)
			{
				RCLCPP_INFO(
					this->get_logger(),
					"服务成功: %s, current_state=%s",
					response->message.c_str(), response->current_state.c_str());
				return 0;
			}

			RCLCPP_WARN(
				this->get_logger(),
				"服务拒绝: %s, current_state=%s",
				response->message.c_str(), response->current_state.c_str());
			return 2;
		}
		else
		{
			auto client_ = client_get_;
			if (!client_->wait_for_service(std::chrono::milliseconds(timeout_ms_)))
			{
				RCLCPP_ERROR(this->get_logger(), "服务不可用: /robot_dog/get_state");
				return 1;
			}

			auto request = std::make_shared<robot_dog_interfaces::srv::GetRobotState::Request>();

			auto future = client_->async_send_request(request);

			const auto result = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), future,
				std::chrono::milliseconds(timeout_ms_));

			if (result != rclcpp::FutureReturnCode::SUCCESS)
			{
				RCLCPP_ERROR(this->get_logger(), "等待服务响应超时或失败");
				return 1;
			}

			const auto response = future.get();

			RCLCPP_INFO(
				this->get_logger(),
				"服务成功: current_state=%s battery=%.2f",
				response->current_state.c_str(), response->battery_percentage);

			return 0;
		}
		return 0;
	}

private:
	rclcpp::Client<robot_dog_interfaces::srv::SetRobotState>::SharedPtr client_set_;
	rclcpp::Client<robot_dog_interfaces::srv::GetRobotState>::SharedPtr client_get_;

	std::string target_state_;
	int timeout_ms_;
	bool status_flag_;
};

int main(int argc, char *argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		auto node = std::make_shared<StateClient>();
		return_code = node->sendRequest();
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
