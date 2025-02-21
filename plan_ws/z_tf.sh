#!/bin/bash
sleep 3
cd /home/agilex/plan_ws
source ./devel/setup.bash
roslaunch local_plan test.launch
