import cv2
import os
import numpy as np
import sys
import time
import shutil

def pwd():
    path = os.getcwd()
    print(path)
    path = path.replace('\\', '\\\\')
    print(path)
    return path

def is_image_clear(file_path, file_name, threshold=10000):
    ts1 = time.time()
    # 读取图片
    frame =cv2.imdecode(np.fromfile(file_path, dtype=np.uint8), -1)
    gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    # 使用Canny边缘检测
    edges = cv2.Canny(gray_frame, threshold1=80, threshold2=150)

    # 计算边缘数量
    num_edges = np.sum(edges != 0)
    ts2 = time.time() - ts1

    # print(ts2, frame, num_edges)
    # print(ts2, file_name, num_edges)

    return num_edges

    # # 根据边缘数量判断清晰度，这里假设阈值为1000，可以根据实际情况调整
    # if num_edges > 1000:
    #     return True
    # else:
    #     return False



if __name__ == "__main__":


    # 设置文件夹路径
    folder_path = 'E:\农场图片\农场图片\农场图片'

    # 列出文件夹下所有文件和文件夹
    for dirname, subdirs, files in os.walk(folder_path):
        for filename in files:
            print(os.path.join(dirname, filename))

            accu_num = is_image_clear(os.path.join(dirname, filename), "")
            if accu_num > 290000:
                shutil.copy2(os.path.join(dirname, filename), 'E:\\农场图片\\农场图片\\农场图片筛选')