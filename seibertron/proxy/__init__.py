# app.py
from flask import Flask
import asyncio
import threading
from server import webSocketServer
from flask_cors import CORS
import sched
import time

from routers.userRouters import user_bp
from routers.authRouters import auth_bp
from routers.sysRouters import sys_bp
from routers.mapRouters import map_bp
from routers.pathRouters import path_bp
from routers.sseRouters import sse_bp
from routers.cameraRouters import camera_bp
from routers.netRouters import net_bp
from routers.taskRouters import task_bp

websocket = None
app = Flask(__name__)

# 配置路由
app.register_blueprint(auth_bp, url_prefix='/hms/auth')
app.register_blueprint(user_bp, url_prefix='/hms/user')
app.register_blueprint(sse_bp, url_prefix='/hms/sse')
app.register_blueprint(sys_bp, url_prefix='/hms/sys')
app.register_blueprint(map_bp, url_prefix='/hms/map')
app.register_blueprint(path_bp, url_prefix='/hms/path')
app.register_blueprint(camera_bp, url_prefix='/hms/camera')
app.register_blueprint(net_bp, url_prefix='/hms/network')
app.register_blueprint(task_bp, url_prefix='/hms/task')
CORS(app, resources=r'/*')

scheduler = sched.scheduler(time.time, time.sleep)

def start_flask_app():
    app.run(host='0.0.0.0')

if __name__ == "__main__":
    # 启动 Flask 服务器线程
    flask_thread = threading.Thread(target=start_flask_app)
    flask_thread.start()
    scheduler.run()
    asyncio.run(webSocketServer.start_server())
