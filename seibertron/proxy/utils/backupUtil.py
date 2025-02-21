import os
import shutil
import config

back_map_path = config.back_map_path

target_pgo_directory = config.target_pgo_directory

target_pcd_directory = config.target_pcd_directory
target_local_directory = config.target_local_directory
target_control_directory = config.target_control_directory

# 地图切换，切换到file_path号地图
def restore_file(file_path):
    # 检查文件是否存在
    if os.path.isfile(back_map_path + '/map' + file_path + '/target' + file_path + '.json') or os.path.isfile(
            (back_map_path + '/map' + file_path + '/map' + file_path + '.pcd')):
        # 移动文件到目标目录
        try:
            shutil.copy((back_map_path + '/map' + file_path + '/map' + file_path + '.pcd'), target_pcd_directory)
        except Exception as e:
            print(e)
        try:
            shutil.copy((back_map_path + '/map' + file_path + '/target' + file_path + '.json'), target_local_directory)
            shutil.copy((back_map_path + '/map' + file_path + '/target' + file_path + '.json'), target_control_directory)

        except Exception as e:
            print(e)

        print(f"文件恢复成功")
        return True
    else:
        print(f'地图不存在')
        return False

# 地图备份
def backup_map_file(file_path):

    # 检查文件是否存在
    if os.path.isfile(target_pcd_directory):
        # 确保目标目录存在，如果不存在则创建
        if not os.path.exists(back_map_path):
            os.makedirs(back_map_path)
            # 移动文件到目标目录
        shutil.copy(target_pgo_directory, target_pcd_directory)
        shutil.copy(target_pcd_directory, (back_map_path + '/map' + file_path + '/map' + file_path + '.pcd'))
        print(f"文件已备份成功")
        return back_map_path + '/map' + file_path + '/map' + file_path + '.pcd'
    else:
        print("文件不存在，程序结束")
        return '文件备份失败'

# 路径保存
def backup_json_file(file_path):
    # 检查文件是否存在
    if os.path.isfile(target_local_directory):
        # 确保目标目录存在，如果不存在则创建
        if not os.path.exists(back_map_path):
            os.makedirs(back_map_path)
            # 移动文件到目标目录
        shutil.copy(target_local_directory, back_map_path+ '/map' + file_path + '/target' + file_path + '.json')

        print(f"文件已备份成功")
    else:
        print("文件不存在，程序结束")

# 备份路径恢复
def restore_json_file(index, file_name):
    restore_file_path = back_map_path + '/map' + index + '/path/' + file_name + '.json'
    if os.path.isfile(restore_file_path):
        shutil.copy(restore_file_path, back_map_path + '/map' + index + '/target' + index + '.json')
        return True
    return False


# 动态拼接

