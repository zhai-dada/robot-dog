cmake_minimum_required(VERSION 3.8)
project(robot_dog_day9_tf)

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
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

add_executable(tf_broadcaster src/tf_broadcaster.cpp)
ament_target_dependencies(
  tf_broadcaster
  geometry_msgs
  rclcpp
  tf2
  tf2_geometry_msgs
  tf2_ros)

add_executable(tf_listener src/tf_listener.cpp)
ament_target_dependencies(
  tf_listener
  geometry_msgs
  rclcpp
  tf2
  tf2_geometry_msgs
  tf2_ros)

install(TARGETS
  tf_broadcaster
  tf_listener
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
