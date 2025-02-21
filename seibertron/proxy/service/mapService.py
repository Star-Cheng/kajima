# from db import mapdb
import subprocess
import time
import config as config
from utils import backupUtil, fileUtil, configUtil
from ros_sub import ros_cloud, ros_odometry

ws_fastlio = config.ws_fastlio
cn = configUtil.cn
def map_list():
    map_list = fileUtil.find_map_list()
    return map_list

def get_map(index):
    return config.back_map_path + '/map' + index + '/map' + index + '.pcd'

def dense_list(index):
    # 使用示例
    directory_path = config.back_map_path + '/map' + index + '/densely'  # 替换为你的文件夹路径
    file_extension = '.pcd'  # 替换为你想读取的文件格式

    files_info = fileUtil.list_files_in_directory(directory_path, file_extension)
    return files_info

def map_start():
    ros_cloud.start()
    time.sleep(2)
    ros_odometry.start()

    #启动fast_lio 建图
    script_cmd = f"gnome-terminal --geometry=78x12+480+1000 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_fastlio}/devel/setup.bash && roslaunch fast_lio mapping_avia.launch'"
    subprocess.Popen(script_cmd, shell=True)
    #等待一段时间后打开第二个终端，输入另一个命令
    time.sleep(1)
    script_cmd = f"gnome-terminal --geometry=78x12+1200+1000 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_fastlio}/devel/setup.bash && roslaunch aloam_velodyne fastlio_ouster64.launch'"
    subprocess.Popen(script_cmd, shell=True)
    
    
    cn.set_options("carState", "activate_map", "0")
    cn.set_options("carState", "running_state", "1")

def map_stop():
    ros_cloud.stop()
    ros_odometry.stop()
    # 停止建图
    terminate_roslaunch("mapping_avia", "closing_scan")
    terminate_roslaunch("fastlio_ouster64", "close_scan")

    cn.set_options("carState", "running_state", "0")

    print('map_stop')
    
def map_backup(map_number):
    # 备份稀疏地图
    backupUtil.backup_map_file(map_number)
    # 备份稠密地图
    source_directory = './targetDirectory/pcd/'  # 替换为你的源目录路径
    destination_directory = config.back_map_path + '/map' + map_number + '/densely'  # 替换为你的目标目录路径
    fileUtil.copy_latest_file(source_directory, '.pcd', destination_directory)
    # 修改汽车状态 激活地图编号
    cn.set_options("carState", "activate_map", str(map_number))
    return True


def map_change(map_number):
    # 修改汽车状态 激活地图编号
    result = backupUtil.restore_file(map_number)
    if result:
        cn.set_options("carState", "activate_map", str(map_number))
        cn.set_options("carState", "running_state", "0")
    return result


def map_delete(map_number):
    fileUtil.delete_all_files_in_directory(config.back_map_path + '/map' + map_number)
    cn.set_options("carState", "activate_map", str(0))
    return True


def terminate_roslaunch(process_name, message):
    # 使用 pkill 终止特定 ROS launch 进程
    subprocess.Popen(f"pkill -f {process_name}", shell=True)
