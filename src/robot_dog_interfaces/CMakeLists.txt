cmake_minimum_required(VERSION 3.8)
project(robot_dog_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotStatus.msg"
  "srv/SetRobotState.srv"
  "srv/GetRobotState.srv"
  "action/ExecuteGait.action"
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
