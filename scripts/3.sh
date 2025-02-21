#!/bin/bash
# ----- kajima3_mapping.sh -----
gnome-terminal --geometry=80x12+470+800 -- bash -c "cd /home/agile/ws_robot/fast_lp; source ./devel/setup.bash; roslaunch fast_lio mapping_avia.launch"
sleep 3
gnome-terminal --geometry=80x12+1200+800 -- bash -c "cd /home/agile/ws_robot/fast_lp; source ./devel/setup.bash; roslaunch aloam_velodyne fastlio_ouster64.launch"
