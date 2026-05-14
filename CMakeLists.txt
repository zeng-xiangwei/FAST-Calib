cmake_minimum_required(VERSION 3.8)
project(fast_calib)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# C++17 required for std::filesystem
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(pcl_conversions REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(livox_ros_driver2 REQUIRED)
find_package(h10_w REQUIRED)

find_package(PCL REQUIRED)
find_package(OpenCV REQUIRED)

# Include directories
include_directories(
  include
  ${PCL_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
)
# Set build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

# Create executable
add_executable(fast_calib src/main.cpp)

# Link libraries
ament_target_dependencies(fast_calib
  rclcpp
  sensor_msgs
  pcl_conversions
  rosbag2_cpp
  livox_ros_driver2
)

target_link_libraries(fast_calib
  ${PCL_LIBRARIES}
  ${OpenCV_LIBRARIES}
)

add_executable(save_image src/save_image.cpp)
ament_target_dependencies(save_image 
  rclcpp
  std_msgs
  sensor_msgs
  cv_bridge)
target_link_libraries(save_image ${OpenCV_LIBRARIES})
target_include_directories(save_image PRIVATE ${OpenCV_INCLUDE_DIRS})

# Install
install(TARGETS
  fast_calib
  save_image
  DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(DIRECTORY
  launch
  config
  rviz_cfg
  DESTINATION share/${PROJECT_NAME}
)

# Install Python scripts
install(PROGRAMS
  scripts/update_camera_intrinsics.py
  scripts/read_low_state.py
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()