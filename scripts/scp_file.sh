#!/bin/bash
ip="192.168.1.180"
user="hms"
pswd="123456"

map_backup="seibertron/proxy/backup/"
scans_path="ws_robot/localization_init/src/FAST_LIO/PCD/"

user_path="/home/"${user}"/"
local_path=${user_path}"デスクトップ/maps/"
map1=${user_path}${map_backup}"map1/*"
map2=${user_path}${map_backup}"map2/*"
map3=${user_path}${map_backup}"map3/*"
directory=${user_path}${scans_path}


last_file=$(sshpass -p ${pswd} ssh ${user}@${ip} "find ${directory} -type f -exec ls -t {} + | head -n 1")
cd ${local_path}
sshpass -p ${pswd} scp ${user}@${ip}:${last_file} ./
cd ${local_path}map1
sshpass -p ${pswd} scp ${user}@${ip}:${map1} ./
cd ${local_path}/map2
sshpass -p ${pswd} scp ${user}@${ip}:${map2} ./
cd ${local_path}/map3
sshpass -p ${pswd} scp ${user}@${ip}:${map3} ./
cd ..

