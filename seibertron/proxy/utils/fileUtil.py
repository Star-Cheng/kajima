import os
import shutil
from datetime import datetime
import config
import shutil

# 读取文件信息、名字、序号、大小、时间
def get_file_info(directory):
    file_array = []
    for i, filename in enumerate(os.listdir(directory), start=1):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path):
            creation_time = os.path.getctime(file_path)
            file_size = os.path.getsize(file_path)
            print(f"index: {i}")
            print(f"Filename: {filename}")
            print(f"file_path: {file_path}")
            print(f"Size: {file_size} bytes")
            print(f"Creation Time: {creation_time}")
            file_array.append({
                "id": i,
                "filename": filename,
                "file_path": file_path,
                "file_size": file_size,
                "creation_time": creation_time
            })
    return file_array

def find_map_list(root_dir = 'backup', file_extension = '.pcd'):
    target_files = []
    # 获取 root_dir 下的直接子目录
    for subdir in os.listdir(root_dir):
        subdir_path = os.path.join(root_dir, subdir)
        # 检查是否是目录
        if os.path.isdir(subdir_path):
            # 遍历子目录中的文件
            map_data = {
                "id": subdir[-1],
                "name": subdir[-1] + '号地图',
                "filename": subdir
            }
            has_target_json = 0
            for filename in os.listdir(subdir_path):
                # 检查文件扩展名
                if filename.lower().endswith(file_extension):
                    file_path = os.path.join(subdir_path, filename)
                    file_size = os.path.getsize(file_path)
                    creation_time = os.path.getctime(file_path)
                    map_data = {
                        "id": subdir[-1],
                        "name": subdir[-1] + '号地图',
                        "filename": filename,
                        "file_path": file_path,
                        "KB": round(file_size / 1024, 2),
                        "MB": round(file_size / 1024 / 1024, 2),
                        "file_size": file_size,
                        "type": 1,
                        "creation_time": creation_time
                    }
                    has_target_json= 1
                if filename.lower().endswith(".json"):
                    has_target_json = 2
            map_data['type'] = has_target_json
            target_files.append(map_data)


    return target_files


def find_latest_file_of_type(directory, file_extension):
    latest_file = None
    latest_file_mtime = None

    # 遍历目录中的文件
    for filename in os.listdir(directory):
        # 检查文件扩展名
        if filename.lower().endswith(file_extension):
            file_path = os.path.join(directory, filename)
            # 检查是否是文件（而不是目录）
            if os.path.isfile(file_path):
                # 获取文件的修改时间
                file_mtime = os.path.getmtime(file_path)
                # 如果这是最新的文件，或者我们还没有找到任何文件
                if latest_file is None or file_mtime > latest_file_mtime:
                    latest_file = file_path
                    latest_file_mtime = file_mtime

    return latest_file


def copy_latest_file(source_dir, file_extension, destination_dir):
    # 找到最新的指定类型文件
    latest_file = find_latest_file_of_type(source_dir, file_extension)

    if latest_file:
        # 确保目标目录存在
        os.makedirs(destination_dir, exist_ok=True)
        # 获取文件名（不包括路径）
        file_name = os.path.basename(latest_file)
        # 构建目标文件路径
        destination_file = os.path.join(destination_dir, file_name)
        # 复制文件
        shutil.copy2(latest_file, destination_file)
        print(f"Copied {latest_file} to {destination_file}")
    else:
        print(f"No files with extension {file_extension} found in {source_dir}")


def delete_all_files_in_directory(directory_path):
    # 检查目录是否存在
    if not os.path.isdir(directory_path):
        print(f"The directory {directory_path} does not exist.")
        return

    # 遍历目录中的每一项
    for root, dirs, files in os.walk(directory_path):
        for file in files:
            file_path = os.path.join(root, file)
            try:
                os.remove(file_path)
                print(f"Deleted file: {file_path}")
            except Exception as e:
                print(f"Failed to delete file {file_path}: {e}")


def copy_file(source_file, destination_dir, new_file_name):
    """
    将指定文件复制到指定目录，并按照指定名称命名。

    参数:
    source_file (str): 源文件路径
    destination_dir (str): 目标目录路径
    new_file_name (str): 新文件名（包含扩展名）
    """
    # 确保目标目录存在，如果不存在则创建
    if not os.path.exists(destination_dir):
        os.makedirs(destination_dir)

    # 构建目标文件路径
    destination_file = os.path.join(destination_dir, new_file_name)

    # 复制文件
    shutil.copy2(source_file, destination_file)
    print(f"文件已复制到: {destination_file}")


def list_files_in_directory(directory, file_extension):
    file_list = []
    file_count = 1
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(file_extension):
                file_path = os.path.join(root, file)
                file_size = os.path.getsize(file_path)
                creation_time = os.path.getctime(file_path)
                file_list.append({
                    'index': file_count,
                    'filename': file,
                    'filepath': file_path,
                    'filesize': file_size,
                    'KB': round(file_size / 1024, 2),
                    "MB": round(file_size / 1024 / 1024, 2),
                    'creation_time': creation_time
                })
                file_count += 1
    return file_list



