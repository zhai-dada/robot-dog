#include <array>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/srv/set_robot_state.hpp"
#include "robot_dog_interfaces/srv/get_robot_state.hpp"

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

		status_set_service_ = this->create_service<robot_dog_interfaces::srv::SetRobotState>(
			"robot_dog/set_state",
			std::bind(
				&StateServer::handleRequestSet,
				this,
				std::placeholders::_1,
				std::placeholders::_2));

		status_get_status_set_service_ = this->create_service<robot_dog_interfaces::srv::GetRobotState>(
			"robot_dog/get_state",
			std::bind(
				&StateServer::handleRequestGet,
				this,
				std::placeholders::_1,
				std::placeholders::_2));

		RCLCPP_INFO(
			this->get_logger(),
			"状态服务端已启动: initial_state=%s service=/robot_dog/set_state",
			current_state_.c_str());
		RCLCPP_INFO(
			this->get_logger(),
			"状态服务端已启动: initial_state=%s service=/robot_dog/get_state",
			current_state_.c_str());
	}

private:
	bool isValidState(const std::string &state) const
	{
		static constexpr std::array<std::string_view, 4> valid_states = {
			"STANDING", "SIT", "CRAWL", "TROT"
		};

		for (const std::string_view valid_state : valid_states)
		{
			if (state == valid_state)
			{
				return true;
			}
		}

		return false;
	}

	void handleRequestSet(
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
		if(current_state_ == "STANDING" && request->target_state == "STANDING")
		{
			response->message = current_state_ + " can not convert to " + request->target_state;
			response->success = false;
			response->current_state = current_state_;
		}
		else if(current_state_ == "SIT" && request->target_state != "STANDING")
		{
			response->message = current_state_ + " can not convert to " + request->target_state;
			response->success = false;
			response->current_state = current_state_;
		}
		else if(current_state_ == "CRAWL" && (request->target_state != "STANDING" && request->target_state != "TROT"))
		{
			response->message = current_state_ + " can not convert to " + request->target_state;
			response->success = false;
			response->current_state = current_state_;
		}
		else if(current_state_ == "TROT" && (request->target_state != "STANDING" && request->target_state != "CRAWL"))
		{
			// throw std::invalid_argument(current_state_ + " can not convert to " + request->target_state);
			response->message = current_state_ + " can not convert to " + request->target_state;
			response->success = false;
			response->current_state = current_state_;
		}
		else
		{
			response->message = "state changed from " + current_state_ + " to " + request->target_state;
			current_state_ = request->target_state;
			response->success = true;
			response->current_state = current_state_;
		}

		RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
	}

	void handleRequestGet(
		const robot_dog_interfaces::srv::GetRobotState::Request::SharedPtr request,
		robot_dog_interfaces::srv::GetRobotState::Response::SharedPtr response)
	{
		response->current_state = current_state_;
		response->battery_percentage = 66.66;
	}

	rclcpp::Service<robot_dog_interfaces::srv::SetRobotState>::SharedPtr status_set_service_;
	rclcpp::Service<robot_dog_interfaces::srv::GetRobotState>::SharedPtr status_get_status_set_service_;

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
		rclcpp::shutdown();
		return_code = 1;
	}

	if (rclcpp::ok())
	{
		rclcpp::shutdown();
	}

	return return_code;
}
