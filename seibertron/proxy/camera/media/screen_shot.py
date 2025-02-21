import cv2
import time
from datetime import datetime
import config
# RTSP流的URL
# rtsp_url = 'rtsp://admin:edge2021@192.168.2.51:554/stream1'

rtsp_url = config.rtsp_ip
save_path = config.picture_path


def snap_shot():
    path = ''
    # 打开RTSP流
    cap = cv2.VideoCapture(rtsp_url)

    # 读取一帧画面
    ret, frame = cap.read()

    # 如果成功读取到一帧画面
    if ret:
        # 保存为图片

        # 检查是否需要存储图片
        current_time = time.time()

        # 格式化 datetime 对象为字符串
        now_time = datetime.fromtimestamp(current_time).strftime('%Y%m%d%H%M%S')
        path = save_path.format(now_time)  # 截图保存路径
        cv2.imwrite(path, frame)
        print("Screenshot saved successfully.")
    else:
        print("Error: Unable to read the frame.")

    # 释放VideoCapture对象
    cap.release()
    return path
