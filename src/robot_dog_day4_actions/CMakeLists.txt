cmake_minimum_required(VERSION 3.8)
project(robot_dog_day4_actions)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(ament_cmake_uncrustify_ADDITIONAL_ARGS
  "-c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../config/robot_dog_code_style.cfg")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(robot_dog_interfaces REQUIRED)

add_executable(gait_action_server src/gait_action_server.cpp)
ament_target_dependencies(gait_action_server rclcpp rclcpp_action robot_dog_interfaces)

add_executable(gait_action_client src/gait_action_client.cpp)
ament_target_dependencies(gait_action_client rclcpp rclcpp_action robot_dog_interfaces)

install(TARGETS
  gait_action_server
  gait_action_client
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
