import threading
from schedule import every, repeat, run_pending
import time
from file import file_utils


# 启动文件自动上传
def start_task():
    # 创建线程 文件上传线程
    thread = threading.Thread(target=init_task)
    # 启动线程 文件上传线程
    thread.start()


def upload_job():
    print(11111)
    file_utils.local_file_upload()

#every(5).minutes.do(upload_job)

def init_task():
    while True:
        run_pending()
        time.sleep(1)





