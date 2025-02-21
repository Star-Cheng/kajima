#!/bin/bash

sleep 10
gnome-terminal -- bash -c "source /opt/ros/noetic/setup.bash && source /home/agile/ws_robot/ws_bunker/devel/setup.bash && /opt/ros/noetic/bin/roslaunch bunker_bringup bunker_robot_base.launch"

exit 0
