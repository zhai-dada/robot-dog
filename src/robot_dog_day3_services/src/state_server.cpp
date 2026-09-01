#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/srv/set_robot_state.hpp"

class StateServer final : public rclcpp::Node
{
public:
	StateServer() : Node("state_server")
	{
		current_state_ = this->declare_parameter<std::string>("initial_state", "STANDING");
		if (!isValidState(current_state_))
		{
			throw std::invalid_argument("initial_state is not a supported robot state");
		}

		service_ = this->create_service<robot_dog_interfaces::srv::SetRobotState>(
			"robot_dog/set_state",
			std::bind(
				&StateServer::handleRequest,
				this,
				std::placeholders::_1,
				std::placeholders::_2));

		RCLCPP_INFO(
			this->get_logger(),
			"状态服务端已启动: initial_state=%s service=/robot_dog/set_state",
			current_state_.c_str());
	}

private:
	bool isValidState(const std::string &state) const
	{
		static constexpr std::array<const char *, 4> valid_states = {
			"STANDING", "SIT", "CRAWL", "TROT"};

		for (const char *valid_state : valid_states)
		{
			if (state == valid_state)
			{
				return true;
			}
		}

		return false;
	}

	void handleRequest(
		const robot_dog_interfaces::srv::SetRobotState::Request::SharedPtr request,
		robot_dog_interfaces::srv::SetRobotState::Response::SharedPtr response)
	{
		if (!isValidState(request->target_state))
		{
			response->success = false;
			response->message = "unsupported target state: " + request->target_state;
			response->current_state = current_state_;
			RCLCPP_WARN(this->get_logger(), "%s", response->message.c_str());
			return;
		}

		const std::string previous_state = current_state_;
		current_state_ = request->target_state;
		response->success = true;
		response->message = "state changed from " + previous_state + " to " + current_state_;
		response->current_state = current_state_;

		RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
	}

	rclcpp::Service<robot_dog_interfaces::srv::SetRobotState>::SharedPtr service_;
	std::string current_state_;
};

int main(int argc, char *argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<StateServer>());
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
