cmake_minimum_required(VERSION 3.8)
project(integ_trajectoire)

set(CMAKE_CXX_STANDARD 17)

find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

ament_auto_add_executable(desired_joint_states src/desired_joint_states.cpp)

target_include_directories(desired_joint_states  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include )

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)


ament_auto_package()
