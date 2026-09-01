#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_dog_interfaces/action/execute_gait.hpp"

class GaitActionClient final : public rclcpp::Node
{
public:
	using ExecuteGait = robot_dog_interfaces::action::ExecuteGait;
	using GoalHandleExecuteGait = rclcpp_action::ClientGoalHandle<ExecuteGait>;

	GaitActionClient()
		: Node("gait_action_client")
	{
		gait_name_ = this->declare_parameter<std::string>("gait_name", "TROT");
		step_count_ = this->declare_parameter<int>("step_count", 5);
		step_period_ms_ = this->declare_parameter<int>("step_period_ms", 300);
		timeout_ms_ = this->declare_parameter<int>("timeout_ms", 3000);

		if (step_count_ <= 0 || step_period_ms_ <= 0 || timeout_ms_ <= 0)
		{
			throw std::invalid_argument("step_count, step_period_ms and timeout_ms are out of range");
		}

		action_client_ = rclcpp_action::create_client<ExecuteGait>(
			this, "robot_dog/execute_gait");
	}

	int sendGoal()
	{
		if (!action_client_->wait_for_action_server(std::chrono::milliseconds(timeout_ms_)))
		{
			RCLCPP_ERROR(this->get_logger(), "Action服务不可用: /robot_dog/execute_gait");
			return 1;
		}

		ExecuteGait::Goal goal;
		goal.gait_name = gait_name_;
		goal.step_count = static_cast<std::uint32_t>(step_count_);
		goal.step_period_ms = static_cast<std::uint32_t>(step_period_ms_);

		RCLCPP_INFO(
			this->get_logger(),
			"发送步态目标: gait=%s steps=%d period_ms=%d",
			gait_name_.c_str(), step_count_, step_period_ms_);

		rclcpp_action::Client<ExecuteGait>::SendGoalOptions options;
		options.feedback_callback = std::bind(
			&GaitActionClient::feedbackCallback,
			this,
			std::placeholders::_1,
			std::placeholders::_2);

		auto goal_future = action_client_->async_send_goal(goal, options);
		const auto goal_result = rclcpp::spin_until_future_complete(
			this->get_node_base_interface(), goal_future,
			std::chrono::milliseconds(timeout_ms_));

		if (goal_result != rclcpp::FutureReturnCode::SUCCESS)
		{
			RCLCPP_ERROR(this->get_logger(), "等待Action目标响应失败");
			return 1;
		}

		auto goal_handle = goal_future.get();
		if (!goal_handle)
		{
			RCLCPP_WARN(this->get_logger(), "Action服务端拒绝目标");
			return 2;
		}

		auto result_future = action_client_->async_get_result(goal_handle);
		const auto result_code = rclcpp::spin_until_future_complete(
			this->get_node_base_interface(), result_future,
			std::chrono::milliseconds(timeout_ms_ + step_count_ * step_period_ms_));

		if (result_code != rclcpp::FutureReturnCode::SUCCESS)
		{
			RCLCPP_ERROR(this->get_logger(), "等待Action结果失败或超时");
			return 1;
		}

		const auto result = result_future.get();
		if (result.code != rclcpp_action::ResultCode::SUCCEEDED)
		{
			RCLCPP_WARN(this->get_logger(), "Action未成功完成");
			return 2;
		}

		RCLCPP_INFO(
			this->get_logger(),
			"Action成功: %s completed_steps=%u",
			result.result->message.c_str(), result.result->completed_steps);
		return result.result->success ? 0 : 2;
	}

private:
	void feedbackCallback(
		GoalHandleExecuteGait::SharedPtr,
		const std::shared_ptr<const ExecuteGait::Feedback> feedback)
	{
		RCLCPP_INFO(
			this->get_logger(),
			"收到反馈: state=%s step=%u/%u",
			feedback->state.c_str(), feedback->current_step, feedback->total_steps);
	}

	rclcpp_action::Client<ExecuteGait>::SharedPtr action_client_;
	std::string gait_name_;
	int step_count_;
	int step_period_ms_;
	int timeout_ms_;
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		auto node = std::make_shared<GaitActionClient>();
		return_code = node->sendGoal();
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
