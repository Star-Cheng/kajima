#!/bin/bash
# ----- kajima2_bunker_sdk.sh -----
user=$(whoami)
cd "/home/"${user}"/ws_robot/bunker_ws"
source ./devel/setup.bash
roslaunch bunker_bringup bunker_robot_base.launch
