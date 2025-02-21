#!/bin/bash
# ----- kajima1_livox.sh -----
user=$(whoami)
cd "/home/"${user}"/ws_livox"
source devel/setup.bash
roslaunch livox_ros_driver2 msg_MID360.launch

