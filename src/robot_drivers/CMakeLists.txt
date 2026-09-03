cmake_minimum_required(VERSION 3.8)
project(robot_drivers)

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
find_package(robot_utils REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(fake_motor_driver src/fake_motor_driver.cpp)
ament_target_dependencies(fake_motor_driver rclcpp robot_utils std_msgs)

install(TARGETS
  fake_motor_driver
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
