#!/bin/bash

source /opt/ros/noetic/setup.bash
source /home/agile/ws_livox/devel/setup.bash
sleep 60
/opt/ros/noetic/bin/roslaunch livox_ros_driver2 msg_MID360.launch 

exit 0
