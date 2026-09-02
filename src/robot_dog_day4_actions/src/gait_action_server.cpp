#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "robot_dog_interfaces/action/execute_gait.hpp"

class GaitActionServer final : public rclcpp::Node
{
public:
	using ExecuteGait = robot_dog_interfaces::action::ExecuteGait;
	using GoalHandleExecuteGait = rclcpp_action::ServerGoalHandle<ExecuteGait>;

	GaitActionServer()
		: Node("gait_action_server")
	{
		action_server_ = rclcpp_action::create_server<ExecuteGait>(
			this,
			"robot_dog/execute_gait",
			std::bind(&GaitActionServer::handleGoal, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&GaitActionServer::handleCancel, this, std::placeholders::_1),
			std::bind(&GaitActionServer::handleAccepted, this, std::placeholders::_1));

		RCLCPP_INFO(this->get_logger(), "步态Action服务端已启动: /robot_dog/execute_gait");
	}

	~GaitActionServer() override
	{
		if (execution_thread_.joinable())
		{
			execution_thread_.join();
		}
	}

private:
	rclcpp_action::GoalResponse handleGoal(
		const rclcpp_action::GoalUUID &,
		std::shared_ptr<const ExecuteGait::Goal> goal)
	{
		if (!isValidGait(goal->gait_name) || goal->step_count == 0 || goal->step_period_ms == 0)
		{
			RCLCPP_WARN(this->get_logger(), "拒绝非法步态目标");
			return rclcpp_action::GoalResponse::REJECT;
		}

		bool expected = false;
		if (!is_executing_.compare_exchange_strong(expected, true))
		{
			RCLCPP_WARN(this->get_logger(), "已有步态目标在执行，拒绝新目标");
			return rclcpp_action::GoalResponse::REJECT;
		}

		RCLCPP_INFO(
			this->get_logger(),
			"接受步态目标: gait=%s steps=%u period_ms=%u",
			goal->gait_name.c_str(), goal->step_count, goal->step_period_ms);
		return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
	}

	rclcpp_action::CancelResponse handleCancel(
		const std::shared_ptr<GoalHandleExecuteGait> goal_handle)
	{
		RCLCPP_INFO(this->get_logger(), "收到取消步态请求");
		(void)goal_handle;
		return rclcpp_action::CancelResponse::ACCEPT;
	}

	void handleAccepted(const std::shared_ptr<GoalHandleExecuteGait> goal_handle)
	{
		if (execution_thread_.joinable())
		{
			execution_thread_.join();
		}

		execution_thread_ = std::thread(
			&GaitActionServer::execute,
			this,
			goal_handle);
	}

	bool isValidGait(const std::string & gait_name) const
	{
		return gait_name == "CRAWL" || gait_name == "WALK" || gait_name == "TROT";
	}

	void execute(const std::shared_ptr<GoalHandleExecuteGait> goal_handle)
	{
		struct ExecutionFlagGuard
		{
			explicit ExecutionFlagGuard(std::atomic<bool> & flag) : flag_(flag)
			{
			}

			~ExecutionFlagGuard()
			{
				flag_.store(false);
			}

			std::atomic<bool> & flag_;
		} execution_flag_guard(is_executing_);

		auto result = std::make_shared<ExecuteGait::Result>();

		try
		{
			const auto goal = goal_handle->get_goal();
			auto feedback = std::make_shared<ExecuteGait::Feedback>();

			for (std::uint32_t step = 1; step <= goal->step_count; ++step)
			{
				if (goal_handle->is_canceling())
				{
					result->success = false;
					result->message = "gait execution canceled";
					result->completed_steps = step - 1;
					goal_handle->canceled(result);
					RCLCPP_INFO(
						this->get_logger(),
						"步态执行已取消: completed_steps=%u",
						result->completed_steps);
					return;
				}

				feedback->current_step = step;
				feedback->total_steps = goal->step_count;
				feedback->state = "EXECUTING_" + goal->gait_name;
				goal_handle->publish_feedback(feedback);

				RCLCPP_INFO(
					this->get_logger(),
					"步态反馈: gait=%s step=%u/%u",
					goal->gait_name.c_str(), step, goal->step_count);

				const auto deadline = std::chrono::steady_clock::now() +
					std::chrono::milliseconds(goal->step_period_ms);
				while (std::chrono::steady_clock::now() < deadline)
				{
					if (goal_handle->is_canceling())
					{
						result->success = false;
						result->message = "gait execution canceled";
						result->completed_steps = step - 1;
						goal_handle->canceled(result);
						RCLCPP_INFO(
							this->get_logger(),
							"步态执行已取消: completed_steps=%u",
							result->completed_steps);
						return;
					}

					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			}

			result->success = true;
			result->message = "gait execution completed: " + goal->gait_name;
			result->completed_steps = goal->step_count;
			goal_handle->succeed(result);
			RCLCPP_INFO(this->get_logger(), "%s", result->message.c_str());
		}
		catch (const std::exception & e)
		{
			result->success = false;
			result->message = "gait execution failed: " + std::string(e.what());
			result->completed_steps = 0;

			if (goal_handle->is_canceling())
			{
				goal_handle->canceled(result);
			}
			else
			{
				goal_handle->abort(result);
			}

			RCLCPP_ERROR(this->get_logger(), "%s", result->message.c_str());
		}
	}

	rclcpp_action::Server<ExecuteGait>::SharedPtr action_server_;
	std::thread execution_thread_;
	std::atomic<bool> is_executing_{false};
};

int main(int argc, char * argv[])
{
	rclcpp::init(argc, argv);
	int return_code = 0;

	try
	{
		rclcpp::spin(std::make_shared<GaitActionServer>());
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
