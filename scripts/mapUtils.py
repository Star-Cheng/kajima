import os  
import shutil  

back_map_path = '../localization_init/src/'

target_pcd_directory = '../localization_init/src/FAST_LIO/pcd_L/update_map.pcd'
target_local_directory = '../localization_init/src/json/target.json'
target_control_directory = '../control_ws/src/bunker_control/src/target.json'


def restore_file(file_path):
    # 检查文件是否存在  
    print(back_map_path, '/target' + file_path + '.json')
    if os.path.isfile(back_map_path + '/target' + file_path + '.json') and os.path.isfile((back_map_path + '/map' + file_path + '.pcd')):  

        # 移动文件到目标目录  
        shutil.copy((back_map_path + '/map' + file_path + '.pcd'), target_pcd_directory)  
        shutil.copy((back_map_path + '/target' + file_path + '.json'), target_local_directory)  
        shutil.copy((back_map_path + '/target' + file_path + '.json'), target_control_directory)  
        print(f"文件恢复成功")  
    else:  

        print("文件不存在，程序结束")


def backup_file(file_path):
    # 检查文件是否存在  

    if os.path.isfile(target_pcd_directory):  
        # 确保目标目录存在，如果不存在则创建  
        if not os.path.exists(back_map_path):  
            os.makedirs(back_map_path)  
        # 移动文件到目标目录  

        shutil.copy(target_pcd_directory, back_map_path + '/map' + file_path + '.pcd')  
        shutil.move(target_local_directory, back_map_path + '/target' + file_path + '.json')  

        print(f"文件已备份成功")  
    else:  

        print("文件不存在，程序结束")