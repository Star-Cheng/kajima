import tkinter as tk
from tkinter import scrolledtext, filedialog, simpledialog, PhotoImage, ttk
import subprocess
import os
import time
import subTopic
import mapUtils

'''
def launch_roslaunch(command):
    global workspace
    # 启动 ROS launch 文件
    subprocess.Popen(
        #f"source /opt/ros/noetic/setup.bash && source {workspace}/devel/setup.bash && {command}",
        f"{command}",
        shell=True,
        executable="/bin/bash"
    )

def select_and_restore_map(): 
    global workspace
    default_path = f"{workspace}/scripts/bak"
    # 弹出窗口选择PCD文件  
    filepath = filedialog.askopenfilename(filetypes=[("PCD files", "*.pcd")], initialdir=default_path)  
      
    # 如果用户选择了文件，则显示文件路径  
    if filepath:  
        # 获取不含路径和后缀的文件名  
        filename_without_extension = os.path.splitext(os.path.basename(filepath))[0]  
        
        # 创建新窗口显示文件路径  
        dialog_window = tk.Toplevel(root)  
        dialog_window.title("PCD文件")  
        dialog_window.geometry("300x100")  
          
        tk.Label(dialog_window, text=f"选中的PCD文件：{filename_without_extension}").pack(pady=10)  
          
        # 确定按钮
        def on_ok():
            print("确定按钮被点击")
            script_restore_map(filename_without_extension)
            dialog_window.destroy()
          
        # 取消按钮
        def on_cancel():
            print("取消按钮被点击")
            dialog_window.destroy()  
          
        tk.Button(dialog_window, text="确定", command=on_ok).pack(side=tk.LEFT, padx=10)  
        tk.Button(dialog_window, text="取消", command=on_cancel).pack(side=tk.RIGHT, padx=10)  
	
def select_and_backup_map():
    # UI 界面备份地图
    global workspace
    default_folder = f"{workspace}/scripts/bak"  
    # 直接让用户输入文件名，因为文件夹路径已指定  
    new_filename = simpledialog.askstring("输入文件名", "请输入要备份的地图文件名（不含扩展名）:")  
      
    if new_filename:  
        # 执行bak.sh脚本  
        scripts_backup_map(new_filename)  
        show_message_auto_close(f"正在备份地图：{new_filename}", 2000)  
    else:  
        show_message_auto_close(f"请输入文件名！", 2000)  

def scripts_backup_map(map):
    # 调用脚本备份地图
    global workspace
    script_cmd = f"{workspace}/scripts/bak.sh {workspace} {map}"
    launch_bash(script_cmd)

def script_restore_map(map):
    # 调用脚本恢复地图
    global workspace
    script_cmd = f"{workspace}/scripts/restore.sh {workspace} {map}"
    launch_script(script_cmd)

def launch_nav():
    #启动路径规划
    launch_roslaunch("roslaunch sentry_nav sentry_movebase.launch")

def script_pgm():
    global workspace
    script_cmd = f"{workspace}/scripts/save_pgm.sh"
    launch_bash(script_cmd)

def terminate_nav():
    # 关闭导航
    terminate_roslaunch("sentry_movebase","正在关闭导航...")
'''

def show_message_auto_close(message, duration=2000):
    selected_language = language_var.get()
    # 信息提示弹出窗口
    popup = tk.Toplevel()
    #popup.title("提示")
    popup.title(translations[selected_language]['tip']) 
    
    # 设置初始尺寸并使得窗口居中
    width, height = 300, 100  # 假设提示框的固定尺寸
    screen_width = popup.winfo_screenwidth()
    screen_height = popup.winfo_screenheight()
    x = (screen_width // 2) - (width // 2)
    y = (screen_height // 2) - (height // 2)
    popup.geometry(f"{width}x{height}+{x}+{y}")
    
    # 创建提示标签
    #label = tk.Label(popup, text=message, font=('Helvetica', 14))
    label = tk.Label(popup, text=translations[selected_language][message], font=('Helvetica', 14))
    label.pack(padx=20, pady=20)

    # 设置自动关闭
    popup.after(duration, popup.destroy)


def launch_bash(command):
    subprocess.Popen(
        f"{command}",
        shell=True,
        executable="/bin/bash"
    )

def launch_script(script):
    # 运行脚本
    subprocess.Popen(f"bash {script}", shell=True)

def terminate_roslaunch(process_name, message):
    # 使用 pkill 终止特定 ROS launch 进程
    subprocess.Popen(f"pkill -f {process_name}", shell=True)
    show_message_auto_close(message)

def launch_rs_livox():
    global ws_livox
    # show_message_auto_close(ws_livox)
    #启动雷达驱动
    script_cmd = f"gnome-terminal --geometry=80x24+1000+10 -- bash -c 'source {ws_livox}/devel/setup.bash && roslaunch livox_ros_driver2 msg_MID360.launch'"
    subprocess.Popen(script_cmd, shell=True)

def launch_fast_lio():
    global is_scan
    selected_language = language_var.get()  
    if is_scan:
        #停止建图        
        terminate_roslaunch("mapping_avia","closing_scan")        
        terminate_roslaunch("fastlio_ouster64","close_scan")
        launch_fast_lio.config(text=translations[selected_language]['scan_map'])
        mapUtils.backup_file(map_number)
    else:
        #启动fast_lio 建图
        script_cmd = f"gnome-terminal --geometry=78x12+480+1000 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_fastlio}/devel/setup.bash && roslaunch fast_lio mapping_avia.launch'"
        subprocess.Popen(script_cmd, shell=True)
        #等待一段时间后打开第二个终端，输入另一个命令
        time.sleep(1)
        script_cmd = f"gnome-terminal --geometry=78x12+1200+1000 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_fastlio}/devel/setup.bash && roslaunch aloam_velodyne fastlio_ouster64.launch'"
        subprocess.Popen(script_cmd, shell=True)
        time.sleep(2)
        launch_fast_lio.config(text=translations[selected_language]['stop_scan'])
    is_scan = not is_scan

# def terminate_fast_lio():
#     #停止建图
#     terminate_roslaunch("mapping_avia","closing_scan")
#     terminate_roslaunch("fastlio_ouster64","close_scan")

def launch_nav_start():
    #移动 底盘SDK 驱动
    global ws_bunker,ws_control
    script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_bunker}/devel/setup.bash && roslaunch bunker_bringup bunker_robot_base.launch'"
    subprocess.Popen(script_cmd, shell=True)

def launch_start_stop():
    #启动 底盘运动
    global ws_control, is_started
    selected_language = language_var.get()
    if is_started:
        terminate_roslaunch("demo05","closing")
        start_stop_button.config(text=translations[selected_language]['start'])
    else:
        script_cmd = f"gnome-terminal --geometry=68x14+1280+800 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_control}/devel/setup.bash && rosrun bunker_control demo05'"
        subprocess.Popen(script_cmd, shell=True)
        start_stop_button.config(text=translations[selected_language]['stop'])
    is_started = not is_started


def launch_location():
    global is_nav
    selected_language = language_var.get() 
    if is_nav:
        #关闭重定位
        terminate_roslaunch("localization_mid360","closing_localization1")
        time.sleep(0.5)
        terminate_roslaunch("pose_tran","closing_localization2")
        time.sleep(0.5)
        terminate_roslaunch("obs_get","close_localization")
    else:
        script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_location}/devel/setup.bash && roslaunch fast_lio localization_mid360.launch'"
        subprocess.Popen(script_cmd, shell=True)
        # 等待一段时间后执行其他相关命令
        time.sleep(3)
        script_cmd = f"gnome-terminal --geometry=68x14+1280+480 -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_location}/devel/setup.bash && rosrun fast_lio pose_tran'"
        subprocess.Popen(script_cmd, shell=True)
        time.sleep(3)
        #script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_location}/devel/setup.bash && rosrun fast_lio obs_get'"
        #subprocess.Popen(script_cmd, shell=True)
        #script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_bunker}/devel/setup.bash && rosrun bunker_bringup bringup_can2usb.bash'"
        #subprocess.Popen(script_cmd, shell=True)
        launch_bunker
        # launch_nav_start

def terminate_location():
    #关闭重定位
    terminate_roslaunch("localization_mid360","closing_localization1")
    time.sleep(0.5)
    terminate_roslaunch("pose_tran","closing_localization2")
    time.sleep(0.5)
    terminate_roslaunch("obs_get","close_localization")

def launch_pcdsave():
    script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_location}/devel/setup.bash && rosrun fast_lio pcd_save'"
    subprocess.Popen(script_cmd, shell=True)

def launch_bunker():
    #启动 底盘运动
    global ws_bunker
    script_cmd = f"gnome-terminal -- bash -c 'source /opt/ros/noetic/setup.bash && source {ws_bunker}/devel/setup.bash && rosrun bunker_bringup bringup_can2usb.bash'"
    subprocess.Popen(script_cmd, shell=True)

def terminate_pcdsave():
    terminate_roslaunch("pcd_save","closing_save_map")

def close_terminal():
    os.system("pkill gnome-terminal")  # 终止所有gnome-terminal进程
    terminate_roslaunch("rviz","closing")
    terminate_roslaunch("one-touch","closing")

# 根据选定的语言更新界面  
def on_language_change(event=None):  
    selected_language = language_var.get()  
    # 更新窗口标题  
    root.title(translations[selected_language]['title'])  
    # 更新其他界面元素...  
    # 例如，更新一个标签的文本  
    label_title.config(text=translations[selected_language]['title'])
    if is_scan:
        launch_fast_lio.config(text=translations[selected_language]['stop_scan'])
    else:
        launch_fast_lio.config(text=translations[selected_language]['scan_map']) 
    # terminate_fast_lio.config(text=translations[selected_language]['stop_scan']) 
    trace_bn.config(text=translations[selected_language]['trace'])
    trace_view_bn.config(text=translations[selected_language]['trace_view'])
    if is_nav:
        launch_location.config(text=translations[selected_language]['nav_stop']) 
    else:
        launch_location.config(text=translations[selected_language]['nav_start']) 
    terminate_location.config(text=translations[selected_language]['nav_stop']) 
    launch_nav_start.config(text=translations[selected_language]['sdk']) 
    if is_started:
        start_stop_button.config(text=translations[selected_language]['stop'])
    else:
        start_stop_button.config(text=translations[selected_language]['start'])
    ptz_bn.config(text=translations[selected_language]['ptz'])
    vslam_bn.config(text=translations[selected_language]['vslam'])
    upload_navigation_map.config(text=translations[selected_language]['upload_nav_map'])
    upload_dense_map.config(text=translations[selected_language]['upload_pointcloud'])
    delete_dense_map.config(text=translations[selected_language]['del_pointcloud'])
    bn_close.config(text=translations[selected_language]['close'])

def trace_view():
    subprocess.Popen(['python3', './traceView.py'], text=True)
    print('trace_view打开成功！')

def record_trace():
    subTopic.processes.set()

def launch_ptz():
    subprocess.Popen(['python3', './car/main.py'], text=True)
    print('PTZ打开成功！')

def launch_vslam():
    script_cmd = f"gnome-terminal -- bash -c 'source /home/agile/.bashrc && export LD_LIBRARY_PATH=/home/agile/vslam/lib   && /home/agile/vslam/lib/slam_and_cslam'"
    # script_cmd = f"gnome-terminal -- bash -c 'source /home/hms/.bashrc && export LD_LIBRARY_PATH=/home/hms/singraybot/hms_robot/vslam/lib && /home/hms/singraybot/hms_robot/vslam/lib/slams.sh '"
    subprocess.Popen(script_cmd, shell=True)    

    time.sleep(5)

    # 示例：获取名为 "Example" 的窗口 ID，并将其移动到 (100, 100) 位置，大小为 800x600  
    window_name = "3D-Viewer"  # 替换为你的窗口名称  
    window_id = get_window_id(window_name)  
    if window_id:  
        # move_window(window_id, 1000, 400, 800, 600)  
        subprocess.run(f"wmctrl -ir {window_id} -e 0,1000,400,640,800", shell=True, capture_output=True, text=True) 
        print(f"Moved window '{window_name}' to (100, 100) with size 800x600")  
    else:  
        print(f"Window '{window_name}' not found")


def get_config_value(filename, key):  
    """  
    从指定文件中读取并返回给定键的值。  
  
    参数:  
    filename (str): 包含配置的文件名。  
    key (str): 要查找的配置键名（不包括等号）。  
  
    返回:  
    str or None: 如果找到键，则返回其值（去除前后空白字符）；如果没有找到，则返回None。  
    """  
    value = None  # 初始化返回值为None  
    with open(filename, 'r') as file:  
        for line in file:  
            # 去除行首和行尾的空白字符，然后检查是否包含指定的键  
            stripped_line = line.strip()  
            if stripped_line.startswith(key + '='):  
                # 如果找到，则分割字符串并返回等号后面的部分（去除前后空白字符）  
                value = stripped_line.split('=')[1].strip()  
                print(key, ":", value)
                break  # 找到后退出循环  
    return value  

#from Frank
def upload_navigation_map():
    global workspace, ws_location
    # 定义要上传的文件路径
    source_file = f"{ws_fastlio}/src/FAST_LIO_LC-master/PGO/pcd/update_map.pcd"
    #destination_host = "192.168.1.10"
    #destination_path = "~/"
    destination_file = f"{ws_location}/src/FAST_LIO/pcd_L/update_map.pcd"


    # 构建scp命令
    scp_command = f"sshpass -p 123456 scp {source_file} {master_ip}:~/"
    # 复制文件到远程主机
    subprocess.Popen(scp_command, shell=True)

    # 复制文件到不同工作空间的本地路径
    local_copy_command = f"cp {source_file} {destination_file}"
    subprocess.Popen(local_copy_command, shell=True)

    # 显示上传成功的提示信息
    #show_message_auto_close("ナビ用マップが正常にアップロードされました！", 2000)
    # up_map_ok: 多国语言版 标语， 要查表看实际内容
    show_message_auto_close("up_map_ok", 2000)
   
def upload_dense_map():
    global ws_location, master_ip
    # 打开文件选择对话框
    filepath = filedialog.askopenfilename(title="选择稠密地图", initialdir=f"{ws_location}/src/FAST_LIO/PCD", filetypes=[("PCD files", "*.pcd")])
    print("稠密地图本地路径：",filepath)
    if filepath:
        # 构建scp命令
        print("master ip:", master_ip)
        scp_command = f"sshpass -p 123456 scp {filepath} {master_ip}:~/"
        subprocess.Popen(scp_command, shell=True)
        show_message_auto_close("up_3dmap_ok", 2000)
    else:
        show_message_auto_close("unselect_file", 2000)

def delete_dense_map():
    global ws_location
    # 打开文件选择对话框
    filepath = filedialog.askopenfilename(title="３D地図の選択", initialdir=f"{ws_location}/src/FAST_LIO/PCD", filetypes=[("PCD files", "*.pcd")])
    if filepath:
        # 删除文件
        os.remove(filepath)
        show_message_auto_close("del_3dmap_OK", 2000)
    else:
        show_message_auto_close("unselect_file", 2000)

def upload_path():
    global ws_location, ws_control, master_ip
    # 从远程主机复制picking_list.txt到本地
    scp_command = f"scp {master_ip}:~/picking_list.txt {ws_location}/src/json/"
    subprocess.Popen(scp_command, shell=True)

    # 读取picking_list.txt并更新target.json
    with open(f"{ws_location}/src/json/picking_list.txt", "r") as file:
        lines = file.readlines()
        x, y = map(float, lines[0].strip().split())

    # 更新target.json
    with open(f"{ws_location}/src/json/target.json", "w") as file:
        file.write(f'{{"x": {x}, "y": {y}}}')

    # 复制target.json到控制工作空间
    copy_command = f"cp {ws_location}/src/json/target.json {ws_control}/src/bunker_control/src/target.json"
    subprocess.Popen(copy_command, shell=True)

    show_message_auto_close("up_trace_OK", 2000)
#end Frank 

def button_with_tooltip(parent, bn_button, tooltip_text, tooltip_next_text="nc", val=False):
    selected_language = language_var.get()
    # 创建提示信息标签  
    tooltip = ttk.Label(root, text="", background="white", foreground="black", padding=5, borderwidth=1, relief="solid")  
    tooltip.place_forget()  # 初始隐藏
    def on_enter(event):  
        if val:
            tooltip.config(text=translations[selected_language][tooltip_next_text])  
        else:
            tooltip.config(text=translations[selected_language][tooltip_text])
        # 计算提示信息的位置，这里我们简单地在鼠标下方显示  
        # tooltip_x = event.x_root + 10  # 鼠标X坐标稍微偏右  
        # tooltip_y = event.y_root + 10  # 鼠标Y坐标稍微偏下  
        tooltip_x =  10  # 鼠标X坐标稍微偏右  
        tooltip_y =  event.y_root - 60 # 鼠标Y坐标稍微偏下  
        tooltip.place(x=tooltip_x, y=tooltip_y)  # 显示提示信息 

    def on_leave(event):  
        # 隐藏提示信息  
        tooltip.place_forget()  

    # 绑定事件到按钮  
    bn_button.bind("<Enter>", on_enter)  
    bn_button.bind("<Leave>", on_leave)  
      
    return bn_button  

map_number = '1'
# 标签点击时的回调函数  
def on_tab1_click():  
    update_tab_selection(tab1_button) 
    global map_number
    map_number = '1'
    mapUtils.restore_file(map_number)
  
def on_tab2_click():  
    update_tab_selection(tab2_button)  
    global map_number
    map_number = '2'
    mapUtils.restore_file(map_number)
  
def on_tab3_click():  
    update_tab_selection(tab3_button) 
    global map_number
    map_number = '3'
    mapUtils.restore_file(map_number)
    # 更新标签选中状态的函数  

def update_tab_selection(selected_button):  
    for button in [tab1_button, tab2_button, tab3_button]:  
        if button == selected_button:  
            # 由于ttk.Button没有直接的'selected'状态，我们使用背景色来模拟  
            button.config(bg='#4CAF50')  
        else:  
            # 禁用其他按钮的焦点，并恢复默认背景色  
            button.config(bg='#FFFFFF')  
            # 注意：这里将按钮设置为DISABLED可能不是最佳实践，因为它会改变按钮的外观和交互性。  
            # 一个更好的做法可能是保持所有按钮都可点击，但只通过背景色来区分选中状态。  


class ColorButton:  
    def __init__(self, root):  
        self.colors = ["red", "green", "blue"]  
        self.current_color_index = 0  
  
        self.button = tk.Button(root,   
                                text="",   
                                width=10,   
                                height=5,   
                                bg=self.colors[self.current_color_index],   
                                command=self.change_color)  
        self.button.grid(row=2, column=0, pady=(15,5), padx=30)  
  
    def change_color(self):  
        self.current_color_index = (self.current_color_index + 1) % len(self.colors)  
        self.button.config(bg=self.colors[self.current_color_index]) 

  
def get_window_id(window_name):  
    # 使用 wmctrl -l 列出所有窗口，并使用 grep 过滤窗口名称  
    # 注意：这里使用了 shell=True 来允许管道操作，但这可能会带来安全风险，确保 window_name 是可信的  
    result = subprocess.run(f'wmctrl -l | grep "{window_name}"', shell=True, capture_output=True, text=True)  
      
    # 检查是否有匹配的结果  
    if result.stdout.strip():  
        # 假设第一个匹配的结果就是我们想要的，提取窗口 ID  
        # 窗口 ID 通常是输出的第一列，但具体格式可能因 wmctrl 的版本和输出方式而异  
        # 这里使用 split() 方法假设输出是用空格分隔的  
        window_line = result.stdout.strip().split('\n')[0]  # 取第一行  
        window_id = window_line.split()[0]  # 取第一列作为窗口 ID  
        return window_id  
    else:  
        return None  
  

# def get_window_id(window_name):  
#     # 使用 wmctrl -l 来列出所有窗口，并查找匹配窗口名称的窗口 ID  
#     # script_cmd = f"gnome-terminal -- bash -c 'source /home/hms/.bashrc && export LD_LIBRARY_PATH=/home/hms/singraybot/hms_robot/vslam/lib   && /home/hms/singraybot/hms_robot/vslam/lib/slam_and_cslam'"
#     # # script_cmd = f"gnome-terminal -- bash -c 'source /home/hms/.bashrc && export LD_LIBRARY_PATH=/home/hms/singraybot/hms_robot/vslam/lib && /home/hms/singraybot/hms_robot/vslam/lib/slams.sh '"
#     subprocess.Popen(script_cmd, shell=True)
#     result = subprocess.run(['wmctrl', '-l'], capture_output=True, text=True)  
#     windows = result.stdout.strip().split('\n') 
#     print(result)
#     for window in windows:  
#         parts = window.split()  
#         if len(parts) > 2 and window_name in parts[2]:  
#             return parts[0]  # 窗口 ID 是列出的第一个字段  
#     return None  
  
def move_window(window_id, x, y, width, height):  
    # 使用 wmctrl -r <window_id> -e <mv_x>,<mv_y>,<width>,<height> 来移动和调整窗口大小  
    subprocess.run(['wmctrl', '-r', window_id, '-e', f'0,{x},{y},{width},{height}'])  
  



#workspace = "/home/hms/car5_29"
# 获取系统环境变量 $ws
# 这个方法，在终端中 运行 python one-touch_start.py ， 可以实现。 
# 但是通过 start.desktop 的方式来运行， 则会报错。
# 因为 start.desktop 运行时， 不会运行 .bashrc 。
# workspace = os.getenv('ws')
# workspace = get_config_value("/home/hms/ws_robot/scripts/config","ws")
ws_fastlio = get_config_value("config","ws_fastlio")
ws_location = get_config_value("config","ws_location")
ws_livox = get_config_value("config","ws_livox")
ws_control = get_config_value("config","ws_control")
ws_bunker = get_config_value("config","ws_bunker")
master_ip = get_config_value("config","master_ip")
# 注意:  config 文件是提供相对路径，还是绝对路径。 管理到 启动 python 程序时的路径。 
# 这里：  cd /home/hms/ws_robot 路径。


launch_rs_livox()


# 初始化状态
is_scan = False
is_nav = False
is_started = False


# 假设您有一个包含语言翻译的字典  
translations = {  
    'CN': {'title': 'HMS Robot Manager', 
           'scan_map': '开始扫描建图',
           'stop_scan': '停止建图',
           'trace': '记录导航点',
           'trace_view': '查看路径',           
           'nav_start': '启动导航',
           'nav_stop': '停止导航',
           'sdk': '启动SDK',
           'start': '启动',
           'stop': '停止',
           'ptz': 'ptz',
           'vslam': 'vslam',
           'upload_nav_map': '上传导航地图',
           'upload_pointcloud': '上传3D稠密点云',
           'del_pointcloud': '删除3D稠密点云',
           'close': '关闭',
           'tip': '提示',
           'nc': '',
           'unselect_file': '未选择文件！',
           'up_3dmap_ok': '3D地图上传成功！',
           'del_3dmap_OK': '删除3D地图成功！',
           'up_trace_OK': '路径上传成功！',
           'closing_scan': '正在关闭建图...',
           'close_scan': '建图已关闭',
           'closing_localization1': '正在关闭重定位...',
           'closing_localization2': '正在关闭重定位',
           'close_localization': '关闭重定位',
           'closing_save_map': '正在停止保存地图...',
           'closing': '正在关闭...',
           'up_map_ok': '导航地图上传成功！'},  
    'JP': {'title': 'HMS Robot Manager', 
           'scan_map': '初期マップ作成開始',
           'stop_scan': '初期マップ作成終了',
           'trace': 'レコードナビゲーションポイント',
           'trace_view': 'パスの表示',
           'nav_start': '自律走行開始',
           'nav_stop': '自律走行終了',
           'sdk': 'ナビゲーションの開始',
           'start': 'スタートアップ',
           'stop': 'ストップ',
           'ptz': 'ptz',
           'vslam': 'vslam',
           'upload_nav_map': 'ナビ用マップのアップロード ',
           'upload_pointcloud': '３D地図のアップロード',
           'del_pointcloud': '３D地図の削除 ',
           'close': '閉じる',
           'tip': 'ヒント',
           'nc': '',
           'unselect_file': 'ファイルが選択されていません！',
           'up_3dmap_ok': '３D地図のアップロードに成功しました！',
           'del_3dmap_OK': '３D地図の削除に成功！',
           'up_trace_OK': 'パスアップロードに成功しました！',
           'closing_scan': '建設図を閉じています...',
           'close_scan': '建図が閉じられました',
           'closing_localization1': '再配置をオフにしています...',
           'closing_localization2': '再配置をオフにしています',
           'close_localization': '再配置を閉じる',
           'closing_save_map': '地図の保存を中止しています...',
           'closing': '閉じています...',
           'up_map_ok': 'ナビ用マップが正常にアップロードされました！'},  
    # 添加其他语言...  
}

# 初始化 订阅
subTopic.start()

# 创建 GUI
root = tk.Tk()
root.title("HMS Robot Manager")
root.geometry("400x1200+0+0")

icon = PhotoImage(file='./icon/icon_64x64.png')  
root.tk.call('wm', 'iconphoto', root._w, icon)

label_font = ('Helvetica', 20, 'bold')
button_font = ('Helvetica', 16)

# 创建并放置下拉菜单（Combobox） , 语言选择栏
language_var = tk.StringVar()  # 创建一个StringVar变量来存储选中的语言  
language_var.set("JP")  # 设置默认选中的语言为JP  

language_combobox = ttk.Combobox(root, textvariable=language_var, font=button_font, width=3)  
language_combobox['values'] = ("CN", "JP")  # 设置下拉菜单的选项  
language_combobox.grid(row=0, column=0, padx=(300,0), pady=(10, 0))  # 放置下拉菜单（这里放在标签旁边，可以根据需要调整位置）  
# 配置 Combobox 以右对齐文本（这影响选中的值显示）  
# language_combobox.configure(anchor='e')  # 'e' 表示东（右）

language_var.trace("w", lambda name, index, mode: on_language_change(language_var.get()))


# 加载logo图片
#logo_img = PhotoImage(file="icon.gif")  # 替换为您的logo图片路径
#logo_label = tk.Label(root, image=logo_img)
#logo_label.image = logo_img  # 保持对图片的引用
#logo_label.grid(row=0, column=0, padx=10, pady=10)

# UI 标签
label_title = tk.Label(root, text="HMS Robot Manager", font=label_font)
label_title.grid(row=1, columnspan=1, pady=(5,10))

# 使用 grid 平行放置按钮

# color_button = ColorButton(root)
    # 创建模拟标签的按钮，并初始化第一个标签为选中状态  

global tab1_button, tab2_button, tab3_button  # 使用global来在函数之间共享变量  

tab_frame = tk.Frame(root)
tab_frame.grid(row=2, column=0, pady=(15,5), padx=30)  

tab1_button = tk.Button(tab_frame, text="1号", command=on_tab1_click)
tab1_button.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5, pady=5)
tab1_button.config(bg='#4CAF50')  # 初始化时设置为选中背景色  
tab2_button = tk.Button(tab_frame, text="2号", command=on_tab2_click)
tab2_button.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5, pady=5)
tab3_button = tk.Button(tab_frame, text="3号", command=on_tab3_click)
tab3_button.pack(side=tk.LEFT, expand=True, fill=tk.X, padx=5, pady=5)

# 扫描建图
launch_fast_lio = tk.Button(root, text="初期マップ作成開始", command=launch_fast_lio, font=button_font, width=25)
launch_fast_lio.grid(row=3, column=0, pady=(15,5), padx=30)
# terminate_fast_lio = tk.Button(root, text="初期マップ作成終了", command=terminate_fast_lio, font=button_font, width=25)
# terminate_fast_lio.grid(row=4, column=0, pady=(5,15), padx=30)

# 路径规划
trace_bn = tk.Button(root, text="レコードナビゲーションポイント", command=record_trace, font=button_font, width=25)
trace_bn.grid(row=5, column=0, pady=(15,5), padx=30)
trace_view_bn = tk.Button(root, text="パスの表示", command=trace_view, font=button_font, width=25)
trace_view_bn.grid(row=6, column=0, pady=(5,15), padx=30)

# 自动巡检
launch_location = tk.Button(root, text="自律走行開始", command=launch_location, font=button_font, width=25)
launch_location.grid(row=7, column=0, pady=(15,5), padx=30)
terminate_location = tk.Button(root, text="自律走行終了", command=terminate_location, font=button_font, width=25)
terminate_location.grid(row=8, column=0, pady=(5,15), padx=30)

# 启动SDK 及 控制
launch_nav_start = tk.Button(root, text="ナビゲーションの開始", command=launch_nav_start, font=button_font, width=25)
launch_nav_start.grid(row=9, column=0, pady=(15,5), padx=30)
start_stop_button = tk.Button(root, text="スタートアップ", command=launch_start_stop, font=button_font, width=25)
start_stop_button.grid(row=10, column=0, pady=(5,15), padx=30)

# PTZ
ptz_bn = tk.Button(root, text="PTZ", command=launch_ptz, font=button_font, width=25)
ptz_bn.grid(row=11, column=0, pady=15, padx=30)


# VSLAM
vslam_bn = tk.Button(root, text="VSLAM", command=launch_vslam, font=button_font, width=25)
vslam_bn.grid(row=12, column=0, pady=15, padx=30)


#from Frank
upload_navigation_map = tk.Button(root, text="ナビ用マップのアップロード ", command=upload_navigation_map, font=button_font, width=25)
upload_navigation_map.grid(row=13, column=0, pady=(15,5), padx=30, columnspan=2)
upload_dense_map = tk.Button(root, text="３D地図のアップロード", command=upload_dense_map, font=button_font, width=25)
upload_dense_map.grid(row=14, column=0, pady=5, padx=30)
delete_dense_map = tk.Button(root, text="３D地図の削除 ", command=delete_dense_map, font=button_font, width=25)
delete_dense_map.grid(row=15, column=0, pady=(5,15), padx=30)
# end Frank

# 关闭
bn_close = tk.Button(root, text="閉じる", command=close_terminal, font=button_font, width=25)
bn_close.grid(row=16, column=0, pady=15, padx=30, columnspan=2)


# 创建提示信息标签  
button_with_tooltip(root, launch_fast_lio, "scan_map", "stop_scan", is_scan)
# button_with_tooltip(root, terminate_fast_lio, "stop_scan")
button_with_tooltip(root, trace_bn, "trace")
button_with_tooltip(root, trace_view_bn, "trace_view")
button_with_tooltip(root, launch_location, "nav_start", "nav_stop", is_nav)
# button_with_tooltip(root, terminate_location, "nav_stop")
button_with_tooltip(root, launch_nav_start, "sdk")
button_with_tooltip(root, start_stop_button, "start", "stop", is_started)
button_with_tooltip(root, ptz_bn, "ptz")
button_with_tooltip(root, vslam_bn, "vslam")
button_with_tooltip(root, upload_navigation_map, "upload_nav_map")
button_with_tooltip(root, upload_dense_map, "upload_pointcloud")
button_with_tooltip(root, delete_dense_map, "del_pointcloud")
button_with_tooltip(root, bn_close, "close")



# 设置背景颜色
#root.configure(bg='#282828')

root.mainloop()
