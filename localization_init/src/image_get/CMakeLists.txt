cmake_minimum_required(VERSION 3.0.2)
project(image_get)

## Compile as C++11, supported in ROS Kinetic and newer
# add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
add_definitions(-DROOT_DIR_IMAGE=\"${CMAKE_CURRENT_SOURCE_DIR}/\")

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  nav_msgs
  pcl_conversions
  pcl_ros
  roscpp
  rospy
  image_transport
  tf
  cv_bridge
  
)

find_package(PCL REQUIRED)
find_package(OpenCV)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES image_get
#  CATKIN_DEPENDS geometry_msgs nav_msgs pcl_conversions pcl_ros roscpp rospy tf
#  DEPENDS system_lib
)

include_directories(
# include
  ${catkin_INCLUDE_DIRS}
  ${PCL_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  include
)

add_executable(project src/project3.cpp)
target_link_libraries(project
        ${catkin_LIBRARIES}
        ${PCL_LIBRARIES}
        ${OpenCV_LIBS}
        )
