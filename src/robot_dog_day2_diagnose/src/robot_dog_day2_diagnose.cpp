/*创建一个C++诊断节点，只在电量低于30%时输出WARN日志。不要修改现有Subscriber.*/

#include <memory>
#include <cinttypes>
#include <functional>
#include "rclcpp/rclcpp.hpp"
#include "robot_dog_interfaces/msg/robot_status.hpp"
#include <iostream>

class StatusDiagnose final : public rclcpp::Node
{
public:
    StatusDiagnose() : Node("status_diagnose")
    {
        diagnose_ = this->create_subscription<robot_dog_interfaces::msg::RobotStatus>(
            "robot_dog/status",
            rclcpp::QoS(10),
            std::bind(&StatusDiagnose::statusCallback, this, std::placeholders::_1)
        );
    }
    void statusCallback(const robot_dog_interfaces::msg::RobotStatus::ConstSharedPtr message)
    {
        if(message->battery_percentage <= 30.0)
        {
            RCLCPP_WARN(this->get_logger(), "接收: robot_name=%s state=%s sequence=%" PRIu64 " battery=%.1f%% temperature=%.2f °C",
                message->robot_name.c_str(), message->state.c_str(), message->sequence,
                message->battery_percentage, message->temperature_celsius);
        }
        return;
    }
private:
    rclcpp::Subscription<robot_dog_interfaces::msg::RobotStatus>::SharedPtr diagnose_;
};

int main(int argc, char* argv[])
{
    int return_code = 0;

    rclcpp::init(argc, argv);

    auto node = std::make_shared<StatusDiagnose>();

    try
    {
        rclcpp::spin(node);
    }
    catch(const std::exception& e)
    {
        RCLCPP_ERROR(rclcpp::get_logger("main"), "节点异常: %s", e.what());
		return_code = 1;
    }

    if(rclcpp::ok())
    {
        rclcpp::shutdown();
    }
    
    return return_code;
}