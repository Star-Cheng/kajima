
from camera.media import screen_shot, media_record
from common import result
import time

def start_recording():
    print(start_recording)
    media_record.record()
    time.sleep(2)
    # 这里用截图来模拟录制，实际中需要更复杂的实现
    media_record.recording_event.clear()
    media_record.recording_event.set()
    # messagebox.showinfo("录制", "录制开始（以截图模拟）...")

def stop_recording():
    print(stop_recording)
    media_record.recording_event.clear()  # 设置事件，表示应该停止录制
    # messagebox.showinfo("录制", "录制停止。")
