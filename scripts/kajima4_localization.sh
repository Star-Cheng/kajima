#!/bin/bash
# ----- kajima4_localization.sh -----
gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; roslaunch fast_lio localization_mid360.launch"
sleep 3
gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun fast_lio pose_tran"
sleep 6
gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun fast_lio obs_get"
sleep 3
gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun image_get project"
