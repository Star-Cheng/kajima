#!/bin/bash
# ----- kajima5_nav.sh -----
gnome-terminal -- bash -c "cd /home/agile/ws_robot/plan_ws; source ./devel/setup.bash; roslaunch local_plan test.launch"
sleep 3
gnome-terminal -- bash -c "cd /home/agile/ws_robot/plan_ws; source ./devel/setup.bash; rosrun local_plan local_teb"
sleep 5
gnome-terminal -- bash -c "cd /home/agile/ws_robot/control_ws; source ./devel/setup.bash; rosrun bunker_control demo17"
