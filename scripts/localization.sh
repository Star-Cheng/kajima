#!/bin/bash

# 获取脚本的完整路径
script_path=$(dirname "$0")
# 如果脚本是以相对路径运行的，那么需要转换为绝对路径
GUI=$(cd "$script_path" && pwd)
echo ${GUI}

#sleep 5

#gnome-terminal -- bash -c "source /home/hms/.bashrc && echo Map_=${Map_} && sleep 5"
gnome-terminal -- bash -c "source /home/agile/.bashrc && source /opt/ros/noetic/setup.bash && cd ${GUI} && python3 ${GUI}/one-touch_start.py"

