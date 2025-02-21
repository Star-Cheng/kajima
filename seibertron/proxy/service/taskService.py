from utils import taskUtil, configUtil
from service import cameraService
import threading
import time
cn = configUtil.cn
def set_camera_task(task_config):

    # 修改任务前，先删除旧任务
    taskUtil.delete_task(cn.get_option("cronTask", "start_task_id"))
    taskUtil.delete_task(cn.get_option("cronTask", "end_task_id"))

    config_start_time = task_config['start_time']
    config_end_time = task_config['end_time']
    task_running = task_config['running']
    # 开始时间，结束时间均不为空时，允许设置任务，为空，则取消定时任务
    if task_running:
        start_time = taskUtil.time_to_cron(config_start_time)
        end_time = taskUtil.time_to_cron(config_end_time)

        start_task_id = taskUtil.add_task(start_time, cameraService.start_recording)
        time.sleep(0.1)
        end_task_id = taskUtil.add_task(end_time, cameraService.stop_recording)

        cn.set_options("cronTask", "start_task_id", start_task_id)
        cn.set_options("cronTask", "end_task_id", end_task_id)

    # 无论结果如何，修改配置任务
    cn.set_options("cronTask", "start_time", config_start_time)
    cn.set_options("cronTask", "end_time", config_end_time)
    cn.set_options("cronTask", "running", str(task_running))
    return True

def get_camera_task():
    start_time = cn.get_option("cronTask", "start_time")
    end_time = cn.get_option("cronTask", "end_time")
    running = cn.get_option("cronTask", "running")
    return {'start_time': start_time, 'end_time': end_time, 'running': running}


def start_camera_task():
    config_start_time = cn.get_option("cronTask", "start_time")
    config_end_time = cn.get_option("cronTask", "end_time")
    config_running = cn.get_option("cronTask", "running")
    print(config_start_time, config_end_time)
    set_camera_task({"start_time": config_start_time, "end_time": config_end_time, "running": config_running})

thread = threading.Thread(target=taskUtil.start_scheduler)
# 启动线程 视频录制线程
thread.daemon = True
thread.start()
time.sleep(1)
start_camera_task()
