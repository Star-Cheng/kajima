#!/bin/bash

# 获取脚本的完整路径
script_path=$(dirname "$0")
# 如果脚本是以相对路径运行的，那么需要转换为绝对路径
GUI=$(cd "$script_path" && pwd)

echo "" > ./kajima.desktop
echo "[Desktop Entry]" > ./kajima.desktop
echo "Version=1.0" >> ./kajima.desktop
echo "Type=Application" >> ./kajima.desktop
echo "Terminal=true" >> ./kajima.desktop
echo "Exec=${GUI}/start.sh" >> ./kajima.desktop
echo "Name=KAJIMA" >> ./kajima.desktop
echo "Comment=ROS_Robot" >> ./kajima.desktop
echo "Icon=${GUI}/icon/icon.png" >> ./kajima.desktop

chmod a+x kajima.desktop setup.sh start.sh
#mv ${GUI}/kajima.desktop ~/Desktop
