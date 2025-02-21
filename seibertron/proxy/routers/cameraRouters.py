
from flask import Blueprint, request, Response
import subprocess
from camera.media import screen_shot, media_record
from camera.onvif_zeep.service.Onvif_hik import Onvif_hik
from common import result
import time
from service import cameraService


camera_bp = Blueprint('camera', __name__)

o = Onvif_hik(ip="192.168.1.10", username="admin", password="edge2021")
lbl_image = None

@camera_bp.route('/stream')
def video_feed():

    cmd = ['ffmpeg', '-i', 'rtsp://admin:edge2021@192.168.1.10', '-c', 'copy', '-f', 'flv', '-']
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
 
    return Response(process.stdout, mimetype='video/x-flv')


@camera_bp.route('/start_recording')
def start_recording():
    cameraService.start_recording()
    return result.success("")

@camera_bp.route('/stop_recording')
def stop_recording():
    cameraService.start_recording()
    # messagebox.showinfo("录制", "录制停止。")
    return result.success("")

@camera_bp.route('/capture')
def capture_screenshot():

    return result.success(screen_shot.snap_shot())

@camera_bp.route('/move')
def move():
    # 模拟鼠标向上移动（需要实际实现）
    x = request.args.get("x")
    y = request.args.get("y")
    o.move(x, y)
    # messagebox.showinfo("移动", "鼠标向上移动（模拟）")
    return result.success("")

@camera_bp.route('/on_stop')
def on_stop():
    o.stop()
    return result.success("")

@camera_bp.route('/zoom')
def zoom_in():
    zoom = request.args.get("zoom")
    # 放大功能（假设为图片或视图的放大，需要实际实现）
    if int(zoom) == 0:
        o.stop_zoom()
    else:
        o.zoom(zoom)
    
    # messagebox.showinfo("缩放", "放大（模拟）")
    return result.success("")

@camera_bp.route('/goto_preset')
def goto_preset():
    preset = request.args.get("preset")
    # 跳转预置点功能
    o.goto_preset(preset)
    return result.success("")

@camera_bp.route('/get_presets')
def get_presets():
    # 跳转预置点功能
    presets = o.get_presets()
    
    preset_array = []
    for preset in presets:
        preset_array.append({
            "name": preset.Name,
            "token": preset.token
        })
    return result.success(preset_array)

@camera_bp.route('/set_preset')
def set_preset():
    preset = request.args.get("preset")
    o.set_presets("preset" + str(preset), preset)
    return result.success("")
