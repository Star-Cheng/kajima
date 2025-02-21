import tkinter as tk
from tkinter import ttk
from media import screen_shot, media_record
import time_utils.timed_tasks as timed_tasks
import numpy as np
from onvif_zeep.service.Onvif_hik import Onvif_hik

o = Onvif_hik(ip="192.168.13.212", username="admin", password="edge2021")
lbl_image = None

def start_recording():
    # 这里用截图来模拟录制，实际中需要更复杂的实现
    media_record.recording_event.clear()
    media_record.recording_event.set()
    # messagebox.showinfo("录制", "录制开始（以截图模拟）...")


def stop_recording():
    media_record.recording_event.clear()  # 设置事件，表示应该停止录制
    # messagebox.showinfo("录制", "录制停止。")


def capture_screenshot():
    # 截取屏幕并显示
    screen_shot.snap_shot()
    # messagebox.showinfo("截图", "截图完成(模拟)。")


def move(x, y):
    # 模拟鼠标向上移动（需要实际实现）
    o.move(x, y)
    # messagebox.showinfo("移动", "鼠标向上移动（模拟）")


def on_release():
    o.stop()


def zoom_in():
    # 放大功能（假设为图片或视图的放大，需要实际实现）
    o.zoom(1)
    # messagebox.showinfo("缩放", "放大（模拟）")


def zoom_out():
    # 缩小功能（假设为图片或视图的缩小，需要实际实现）
    o.zoom(-1)
    # messagebox.showinfo("缩放", "缩小（模拟）")


def goto_preset(prest):
    # 跳转预置点功能
    o.goto_preset(prest)


def set_preset(prest):
    o.set_presets("prest" + str(prest), prest)

def open_new_window():  
    # 创建一个新的顶级窗口  
    new_window = tk.Toplevel(root)  
    new_window.title("video")  
    new_window.geometry("640x430+520+100")  

    frame = np.zeros((240, 320, 3), dtype=np.uint8)  # 初始化帧，大小根据实际情况调整
    lbl_image = tk.Label(new_window)
    lbl_image.grid(row=0, column=0,columnspan=6, sticky='w', padx=10, pady=10)

    media_record.record(lbl_image)

# 根据选定的语言更新界面  
def on_language_change(event=None):  
    selected_language = language_var.get()  
    # 更新窗口标题  
    print(start_record_button)
    start_record_button.config(text=translations[selected_language]['start_record'])
    stop_record_button.config(text=translations[selected_language]['stop_record'])
    screenshot_button.config(text=translations[selected_language]['screenshot'])
    up_button.config(text=translations[selected_language]['up'])
    down_button.config(text=translations[selected_language]['down'])
    left_button.config(text=translations[selected_language]['left'])
    right_button.config(text=translations[selected_language]['right'])
    goto_prest1_button.config(text=translations[selected_language]['goto_prest1'])
    goto_prest2_button.config(text=translations[selected_language]['goto_prest2'])
    goto_prest3_button.config(text=translations[selected_language]['goto_prest3'])
    set_prest1_button.config(text=translations[selected_language]['set_prest1'])
    set_prest2_button.config(text=translations[selected_language]['set_prest2'])
    set_prest3_button.config(text=translations[selected_language]['set_prest3'])


# 假设您有一个包含语言翻译的字典  
translations = {  
    'CN': {
            'title': 'PTZ', 
            'start_record': '开始录制',
            'stop_record': '停止录制',
            'screenshot': '截图',
            'up': '上',
            'down': '下',
            'left': '左',
            'right': '右',
            'goto_prest1': '预置点1',
            'goto_prest2': '预置点2',
            'goto_prest3': '预置点3',
            'set_prest1': '设置预置点1',
            'set_prest2': '设置预置点2',
            'set_prest3': '设置预置点3'
          },  
    'JP': {
            'title': 'PTZ', 
            'start_record': '开始录制',
            'stop_record': '停止录制',
            'screenshot': '截图',
            'up': '上',
            'down': '下',
            'left': '左',
            'right': '右',
            'goto_prest1': '预置点1',
            'goto_prest2': '预置点2',
            'goto_prest3': '预置点3',
            'set_prest1': '设置预置点1',
            'set_prest2': '设置预置点2',
            'set_prest3': '设置预置点3'
          },  
    # 添加其他语言...  
}



# record()
# while True:
#     time_utils.sleep(2)
# filetUpload()
# 启动 Flask 应用和录制视频的线程（注意：这里需要你自己管理线程）
# ...
#timed_tasks.start_task()
button_font = ('Helvetica', 16)
# start_recording()
# 创建主窗口
root = tk.Tk()
root.title("PTZ")
# 创建并放置下拉菜单（Combobox） , 语言选择栏
language_var = tk.StringVar()  # 创建一个StringVar变量来存储选中的语言  
language_var.set("JP")  # 设置默认选中的语言为JP  
language_combobox = ttk.Combobox(root, textvariable=language_var, font=button_font, width=3)  
language_combobox['values'] = ("CN", "JP")  # 设置下拉菜单的选项  
language_combobox.grid(row=0, column=2, padx=10, pady=(10, 0))  # 放置下拉菜单（这里放在标签旁边，可以根据需要调整位置）  
# 配置 Combobox 以右对齐文本（这影响选中的值显示）  
# language_combobox.configure(anchor='e')  # 'e' 表示东（右）
language_var.trace("w", lambda name, index, mode: on_language_change(language_var.get()))

# 添加按钮
start_record_button = tk.Button(root, text="开始录屏", command=start_recording)
start_record_button.grid(row=1, column=0, padx=10, pady=10)
stop_record_button = tk.Button(root, text="停止录屏", command=stop_recording)
stop_record_button.grid(row=1, column=1, padx=10, pady=10)
screenshot_button = tk.Button(root, text="截图", command=capture_screenshot)
screenshot_button.grid(row=1, column=2, padx=10, pady=10)
# tk.Button(root, text="放大", command=lambda: zoom_in()).grid(row=1, column=0, adx=10, pady=10)
# tk.Button(root, text="缩小", command=lambda: zoom_out()).grid(row=1, column=2, padx=10, pady=10)
up_button = tk.Button(root, text="上", command=lambda: move(0, 1))
down_button = tk.Button(root, text="下", command=lambda: move(0, -1))
left_button = tk.Button(root, text="左", command=lambda: move(-1, 0))
right_button = tk.Button(root, text="右", command=lambda: move(1, 0))
up_button.grid(row=3, column=1, padx=10, pady=10)
down_button.grid(row=5, column=1, padx=10, pady=10)
left_button.grid(row=4, column=0, padx=10, pady=10)
right_button.grid(row=4, column=2, padx=10, pady=10)
# up_button.bind("ButtonPress>", move(0, 1))
# down_button.bind("ButtonPress>",  move(0, -1))
# left_button.bind("<ButtonPress>", move(-1, 0))
# right_button.bind("ButtonPress>", move(1, 0))
#
# up_button.bind("<ButtonRelease>", on_release)
# down_button.bind("<ButtonRelease>", on_release)
# left_button.bind("<ButtonRelease>", on_release)
# right_button.bind("<ButtonRelease>", on_release)
goto_prest1_button = tk.Button(root, text="预置点1", command=lambda: goto_preset(1), width=7)
goto_prest1_button.grid(row=6, column=0, padx=3, pady=5)
goto_prest2_button = tk.Button(root, text="预置点2", command=lambda: goto_preset(2), width=7)
goto_prest2_button.grid(row=6, column=1, padx=3, pady=5)
goto_prest3_button = tk.Button(root, text="预置点3", command=lambda: goto_preset(3), width=7)
goto_prest3_button.grid(row=6, column=2, padx=3, pady=5)
set_prest1_button = tk.Button(root, text="设置1", command=lambda: set_preset(1), width=7)
set_prest1_button.grid(row=7, column=0, padx=3, pady=5)
set_prest2_button = tk.Button(root, text="设置2", command=lambda: set_preset(2), width=7)
set_prest2_button.grid(row=7, column=1, padx=3, pady=5)
set_prest3_button = tk.Button(root, text="设置3", command=lambda: set_preset(3), width=7)
set_prest3_button.grid(row=7, column=2, padx=3, pady=5)
open_new_window()
root.mainloop()
# app.run(host='0.0.0.0', port=5000)  # 注意：在生产环境中，不要使用 0.0.0.0
