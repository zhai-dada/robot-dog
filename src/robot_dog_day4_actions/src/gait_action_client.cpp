#include <chrono>
#include <csignal>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_dog_interfaces/action/execute_gait.hpp"

namespace
{
	volatile std::sig_atomic_t interrupt_requested = 0;

	void signalHandler(int)
	{
		interrupt_requested = 1;
	}
}

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
		if (interrupt_requested != 0)
		{
			return 130;
		}

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
		auto goal_result = rclcpp::FutureReturnCode::TIMEOUT;
		const auto goal_deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(timeout_ms_);
		while (goal_result == rclcpp::FutureReturnCode::TIMEOUT && interrupt_requested == 0 &&
			std::chrono::steady_clock::now() < goal_deadline)
		{
			goal_result = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), goal_future,
				std::chrono::milliseconds(100));
		}

		if (interrupt_requested != 0)
		{
			RCLCPP_WARN(this->get_logger(), "收到中断请求，目标尚未确认");
			return 130;
		}

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
		auto result_code = rclcpp::FutureReturnCode::TIMEOUT;
		const auto result_timeout_ms = timeout_ms_ + step_count_ * step_period_ms_;
		const auto result_deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(result_timeout_ms);
		while (result_code == rclcpp::FutureReturnCode::TIMEOUT && interrupt_requested == 0 &&
			std::chrono::steady_clock::now() < result_deadline)
		{
			result_code = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), result_future,
				std::chrono::milliseconds(100));
		}

		if (interrupt_requested != 0)
		{
			RCLCPP_WARN(this->get_logger(), "收到中断请求，正在取消步态目标");
			auto cancel_future = action_client_->async_cancel_goal(goal_handle);
			const auto cancel_code = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), cancel_future,
				std::chrono::milliseconds(timeout_ms_));
			if (cancel_code != rclcpp::FutureReturnCode::SUCCESS)
			{
				RCLCPP_ERROR(this->get_logger(), "等待取消响应失败");
				return 1;
			}

			const auto canceled_result_code = rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), result_future,
				std::chrono::milliseconds(timeout_ms_));
			if (canceled_result_code != rclcpp::FutureReturnCode::SUCCESS)
			{
				RCLCPP_ERROR(this->get_logger(), "等待Canceled结果失败");
				return 1;
			}

			const auto canceled_result = result_future.get();
			RCLCPP_INFO(
				this->get_logger(),
				"取消完成: completed_steps=%u",
				canceled_result.result->completed_steps);
			return 130;
		}

		if (result_code != rclcpp::FutureReturnCode::SUCCESS)
		{
			RCLCPP_ERROR(this->get_logger(), "等待Action结果失败或超时");
			auto cancel_future = action_client_->async_cancel_goal(goal_handle);
			rclcpp::spin_until_future_complete(
				this->get_node_base_interface(), cancel_future,
				std::chrono::milliseconds(timeout_ms_));
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
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
	rclcpp::init(
		argc,
		argv,
		rclcpp::InitOptions(),
		rclcpp::SignalHandlerOptions::None);
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
