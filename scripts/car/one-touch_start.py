import tkinter as tk
import subprocess
import os
import time
from media import media_record


def show_message_auto_close(message, duration=2000):
    popup = tk.Toplevel()
    popup.title("提示")

    # 设置初始尺寸并使得窗口居中
    width, height = 300, 100  # 假设提示框的固定尺寸
    screen_width = popup.winfo_screenwidth()
    screen_height = popup.winfo_screenheight()
    x = (screen_width // 2) - (width // 2)
    y = (screen_height // 2) - (height // 2)
    popup.geometry(f"{width}x{height}+{x}+{y}")

    # 创建提示标签
    label = tk.Label(popup, text=message, font=('Helvetica', 14))
    label.pack(padx=20, pady=20)

    # 设置自动关闭
    popup.after(duration, popup.destroy)


def launch_roslaunch(command):
    global workspace
    # 启动 ROS launch 文件
    subprocess.Popen(
        # f"source /opt/ros/noetic/setup.bash && source {workspace}/devel/setup.bash && {command}",
        f"{command}",
        shell=True,
        executable="/bin/bash"
    )


def terminate_roslaunch(process_name, message):
    # 使用 pkill 终止特定 ROS launch 进程
    subprocess.Popen(f"pkill -f {process_name}", shell=True)
    show_message_auto_close(message)


def launch_script(script):
    # subprocess.Popen(f"bash {script}", shell=True)
    print(1)


def launch_rs_livox():
    # subprocess.Popen(['gnome-terminal', '--', 'bash', '-c', 'roslaunch livox_ros_driver2 msg_MID360.launch'])
    print(1)


def launch_fast_lio():
    launch_roslaunch("roslaunch fast_lio mapping_mid360.launch")


def terminate_fast_lio():
    terminate_roslaunch("mapping_mid360", "正在关闭建图...")


def script_pgm():
    global workspace
    launch_roslaunch(f"{workspace}/scripts/save_pgm.sh")


def launch_location():
    global workspace
    launch_roslaunch("roslaunch fast_lio_localization sentry_localize.launch")
    time.sleep(10)
    launch_roslaunch("rosrun fast_lio_localization publish_initial_pose.py 0 0 0 0 0 0")
    # launch_roslaunch(f"{workspace}/scripts/robot_base.sh")
    script_path = f"{workspace}/scripts/robot_base.sh"
    # subprocess.Popen(['gnome-terminal', '--', 'bash', '-c', script_path])


def terminate_location():
    terminate_roslaunch("sentry_localize", "正在关闭重定位...")


def launch_nav():
    launch_roslaunch("roslaunch sentry_nav sentry_movebase.launch")


def launch_nav_start():
    global start_stop_button
    global is_started
    terminate_roslaunch("sentry_movebase", "正在停止路径规划")
    # subprocess.Popen(f"pkill -f sentry_movebase", shell=True)
    launch_roslaunch("roslaunch sentry_nav sentry_movebase.launch")
    time.sleep(5)
    launch_roslaunch("rosrun fast_lio_localization draw_nav_goal.py")
    time.sleep(5)
    # subprocess.Popen(['gnome-terminal', '--', 'bash', '-c', 'rosrun robot_base robot_base'])
    if (start_stop_button != None):
        start_stop_button.config(text="停止")
        show_message_auto_close("启动")
        is_started = True


def launch_start_stop():
    global start_stop_button
    global is_started
    if (start_stop_button != None):
        if is_started:
            # 调用代码2
            start_stop_button.config(text="启动")
            terminate_roslaunch("robot_base", "正在停止")
        else:
            # 调用代码1
            start_stop_button.config(text="停止")
            terminate_roslaunch("robot_base", "正在启动")
            # subprocess.Popen(['gnome-terminal', '--', 'bash', '-c', 'rosrun robot_base robot_base'])
        is_started = not is_started

def start_stop_recording():

    global start_stop_record_button
    global is_recorded
    if (start_stop_button != None):

        if is_recorded:
            # 调用代码2
            start_stop_record_button.config(text="启动录制")
            media_record.stop_event.set()  # 设置事件，表示应该停止录制
            # terminate_roslaunch("robot_base", "正在停止")
        else:
            # 调用代码1
            start_stop_record_button.config(text="停止录制")
            media_record.stop_event.clear()
            media_record.record()

        is_recorded = not is_recorded


    # 这里用截图来模拟录制，实际中需要更复杂的实现

    # messagebox.showinfo("录制", "录制开始（以截图模拟）...")


def terminate_nav():
    terminate_roslaunch("sentry_movebase", "正在关闭导航...")


def close_terminal():
    os.system("pkill gnome-terminal")  # 终止所有gnome-terminal进程
    terminate_roslaunch("rviz", "正在关闭...")
    terminate_roslaunch("one-touch", "正在关闭...")


workspace = "/home/hms/car5_29"
launch_rs_livox()
# 初始化状态
is_started = False

is_recorded = False

# 创建 GUI
root = tk.Tk()
root.title("ROS Launch Manager")
root.geometry("600x700")

label_font = ('Helvetica', 20, 'bold')
button_font = ('Helvetica', 16)

tk.Label(root, text="请点击按钮：", font=label_font).grid(row=0, columnspan=2, pady=(80, 20))

# 使用 grid 平行放置按钮
tk.Button(root, text="启动建图", command=launch_fast_lio, font=button_font, width=20).grid(row=1, column=0, pady=15, padx=5)
tk.Button(root, text="停止建图", command=terminate_fast_lio, font=button_font, width=20).grid(row=1, column=1, pady=15,
                                                                                          padx=5)

tk.Button(root, text="保存2D地图", command=script_pgm, font=button_font, width=20).grid(row=6, column=0, pady=15, padx=5,
                                                                                    columnspan=2)
tk.Button(root, text="关闭所有终端", command=close_terminal, font=button_font, width=20).grid(row=7, column=0, pady=15,
                                                                                        padx=5, columnspan=2)

tk.Button(root, text="启动重定位", command=launch_location, font=button_font, width=20).grid(row=2, column=0, pady=15,
                                                                                        padx=5)
tk.Button(root, text="停止重定位", command=terminate_location, font=button_font, width=20).grid(row=2, column=1, pady=15,
                                                                                           padx=5)

tk.Button(root, text="启动路径规划", command=launch_nav, font=button_font, width=20).grid(row=3, column=0, pady=15, padx=5)
tk.Button(root, text="停止路径规划", command=terminate_nav, font=button_font, width=20).grid(row=3, column=1, pady=15, padx=5)

tk.Button(root, text="启动导航", command=launch_nav_start, font=button_font, width=20).grid(row=4, column=0, pady=15,
                                                                                        padx=5)
start_stop_button = tk.Button(root, text="启动", command=launch_start_stop, font=button_font, width=20)
start_stop_button.grid(row=4, column=1, pady=15, padx=5)

start_stop_record_button = tk.Button(root, text="启动录制", command=start_stop_recording, font=button_font, width=20)
start_stop_record_button.grid(row=5, column=0, pady=15, padx=5)

# tk.Button(root, text="关闭所有终端", command=close_terminal, font=button_font, width=40, height=3).pack(pady=15)


root.mainloop()
