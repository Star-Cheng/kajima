import cv2
import subprocess
#from file import file_utils
import threading
from datetime import datetime
import time
import config

stop_event = threading.Event()  # 用于控制停止的线程事件

snap_shot = threading.Event()

recording_event = threading.Event() # 控制录像事件

rtsp_url = config.rtsp_ip

formatted_time = ''

out = None

last_time = 0 

save_path = config.picture_path  # 截图保存路径

running = False

# 启动视频录制
def record():
    
    global running
    if running == True: return

    # 获取当前时间的 Unix 时间戳
    timestamp = time.time()

    # 将 Unix 时间戳转换为 datetime 对象
    dt_object = datetime.fromtimestamp(timestamp)

    # 格式化 datetime 对象为字符串
    formatted_time = dt_object.strftime('%Y%m%d%H%M%S')
    # 保存视频的文件名
    video_filename = config.video_path + f'{formatted_time}_'
    # 创建线程 视频录制线程
    thread = threading.Thread(target=pull_rtsp, args=(rtsp_url, video_filename, formatted_time))
    # 启动线程 视频录制线程
    thread.daemon = True
    thread.start()
    running = True


def pull_rtsp(rtsp_url, save_file, formatted_time):
    try:
        cap = cv2.VideoCapture(rtsp_url)
        # 检查是否成功打开
        if not cap.isOpened():
            print("Error opening video stream or file")
            exit()
            # 获取视频的帧率和尺寸
        fps = cap.get(cv2.CAP_PROP_FPS)
        width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
        height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

        fourcc = cv2.VideoWriter_fourcc(*'mp4v')  # 使用MP4V编码，你也可以选择其他编解码器如'avc1'（H.264）
        # out = cv2.VideoWriter(save_file, fourcc, fps, (width, height))


        while (cap.isOpened()):
            ret, frame = cap.read()
            if not ret:
                break
            
            if recording_event.is_set():
                global out
                if (out is None) or (not out.isOpened()):

                    # 初始化变量
                    start_time = time.time()
                    last_time = time.time()
                    minute_count = 0
                    last_file_name = None

                    filename = f"{save_file}.mp4"
                    out = cv2.VideoWriter(filename, fourcc, fps, (width, height))

                # 检查是否需要开启新的文件
                elapsed_time = int(time.time() - start_time)
                current_minute = int(elapsed_time // 60)
                if current_minute != minute_count:
                    if out is not None:
                        out.release()  # 关闭前一个文件
                    # 开启下一个文件前，合并前两个文件
                    # if last_file_name is not None:
                        # merge_video_files([f"{save_file}.mp4", filename], f"{save_file}.mp4")
                    # last_file_name = filename
                    # file_utils.filetUpload(last_file_name, formatted_time)

                    filename = f"{save_file}{minute_count:02d}.mp4"

                    out = cv2.VideoWriter(filename, fourcc, fps, (width, height))
                    minute_count = current_minute
                    # start_time = time.time()  # 重置开始时间

                if frame is not None:
                    # if(time.time() - last_time >= 1):
		            #     # 检查是否需要存储图片
                    #     current_time = time.time()
					# 	# 格式化 datetime 对象为字符串
                    #     now_time = datetime.fromtimestamp(current_time).strftime('%Y%m%d%H%M%S')
                    #     path = save_path.format(now_time)  # 截图保存路径
                    #     cv2.imwrite(path, frame)
                    out.write(frame)
            else:
                if out is not None and out.isOpened():
                    formatted_time = ''
                    out.release()
                    global running
                    running = False
                    return
                    # 完成写入后，释放VideoWriter
        out.release()
        #cv2.destroyAllWindows()
        # file_utils.filetUpload(filename, formatted_time)
        print("Video saved successfully!")
    except Exception as e:
        print(f"An error occurred: {e}")
        # 如果发生异常，尝试释放VideoWriter（如果它已经被初始化）
        if 'out' in locals() and out.isOpened():
            out.release()
            # 你还可以在这里添加额外的错误处理逻辑，如保存当前状态、发送通知等


# 文件合并
def merge_video_files(files: [], merge_name): # type: ignore
    # FFmpeg命令模板用于合并视频
    ffmpeg_merge_cmd = [
        'ffmpeg',
        '-i', files[0],
        '-i', files[1],
        '-c', 'copy',  # 复制流（不重新编码
        merge_name
    ]
    subprocess.run(ffmpeg_merge_cmd, check=True)

