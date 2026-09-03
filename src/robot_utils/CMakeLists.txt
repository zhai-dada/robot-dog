cmake_minimum_required(VERSION 3.8)
project(robot_utils)

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

add_library(robot_utils SHARED
  src/angle_utils.cpp)
target_include_directories(robot_utils PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

install(
  DIRECTORY include/
  DESTINATION include)
install(
  TARGETS robot_utils
  EXPORT robot_utils_targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
ament_export_targets(robot_utils_targets HAS_LIBRARY_TARGET)
ament_export_include_directories(include)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
