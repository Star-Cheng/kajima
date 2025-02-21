import shutil
import os
import json
#from db import pathdb, mapdb
from common import Snowflake
from datetime import datetime
from ros_sub import ros_odometry, ros_cloud
import time
from utils import backupUtil, configUtil, fileUtil
import config
import subprocess

back_map_path = config.back_map_path
target_local_directory = config.target_local_directory
target_control_directory = config.target_control_directory

cn = configUtil.cn

def plan_start():
    ros_cloud.start()
    time.sleep(2)
    ros_odometry.start()
    cn.set_options("carState", "running_state", "2")


def plan_stop():
    ros_odometry.stop()
    ros_cloud.stop()
    # 停止建图
    terminate_roslaunch("mapping_avia", "closing_scan")
    terminate_roslaunch("fastlio_ouster64", "close_scan")
    cn.set_options("carState", "running_state", "0")
    print('map_stop')

def nav_start():
    ros_cloud.start()
    time.sleep(2)
    ros_odometry.start()
    cn.set_options("carState", "running_state", "3")

def nav_stop():
    cn.set_options("carState", "running_state", "0")
    map_number = cn.get_option('carState', 'activa+te_map')
    # 备份稠密地图
    source_directory = './targetDirectory/pcd/'  # 替换为你的源目录路径
    destination_directory = config.back_map_path + '/map' + map_number + '/densely'  # 替换为你的目标目录路径
    fileUtil.copy_latest_file(source_directory, '.pcd', destination_directory)
    ros_odometry.stop()
    ros_cloud.stop()

    terminate_roslaunch("mapping_avia", "closing_scan")
    terminate_roslaunch("fastlio_ouster64", "close_scan")


def path_load(map_number):
    # 读取JSON数据  
    data = read_json_file(back_map_path + '/map' + map_number + '/target' + map_number + '.json')

    try:
        creation_time = os.path.getctime(back_map_path + '/map' + map_number + '/target' + map_number + '.json')
        creation_time_dt = datetime.fromtimestamp(creation_time)
        formatted_time = creation_time_dt.strftime('%Y-%m-%d %H:%M:%S')
    except Exception as e:
        print(e)
        formatted_time = None

    return {
        'map_id': map_number,
        'create_time': formatted_time,
        'data': data
    }

def path_save(map_number, data):
    write_json_file(target_local_directory, data)  
    write_json_file(target_control_directory, data)  
    backupUtil.backup_json_file(str(map_number))

def backup_path(data):
    map_number = str(data['map_id'])
    map_name = data['name']

    path_list = fileUtil.get_file_info(back_map_path + '/map' + map_number + '/path')

    print(path_list)
    for path in path_list:
        if map_name == path['path']:
            return False
        
    source_directory = back_map_path + '/map' + map_number + '/target' + map_number + '.json'   # 替换为你的源目录路径
    destination_directory = back_map_path + '/map' + map_number + '/path'  # 替换为你的目标目录路径

    fileUtil.copy_file(source_directory, destination_directory, map_name + '.json')
    return True

def path_restore(name, map_number):
    return backupUtil.restore_json_file(map_number, name)

def backup_list(map_number):
    directory_path = config.back_map_path + '/map' + map_number + '/path'  # 替换为你的文件夹路径
    file_extension = '.json'  # 替换为你想读取的文件格式

    files_info = fileUtil.list_files_in_directory(directory_path, file_extension)
    return files_info

# 读取JSON文件  
def read_json_file(file_path):
    data = None
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            data = json.load(file)
    except Exception as e:
        print(e)
        data = []

    return data  

# 保存JSON数据到文件  
def write_json_file(file_path, data):  
    with open(file_path, 'w', encoding='utf-8') as file:  
        json.dump(data, file, ensure_ascii=False, indent=4)


def terminate_roslaunch(process_name, message):
    # 使用 pkill 终止特定 ROS launch 进程
    subprocess.Popen(f"pkill -f {process_name}", shell=True)
