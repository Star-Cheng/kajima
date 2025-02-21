from apscheduler.schedulers.background import BackgroundScheduler
from apscheduler.triggers.cron import CronTrigger
from common import Snowflake
import time
import threading
import uuid
from camera.media import screen_shot, media_record

# 创建调度器
scheduler = BackgroundScheduler()

# 存储任务ID与对应的任务信息
task_map = {}

# 动态添加定时任务
def add_task(cron_expression, task_function):
    task_id = str(Snowflake.next_id())  # 使用UUID作为任务的唯一标识符
    trigger = CronTrigger.from_crontab(cron_expression)  # 根据cron表达式生成触发器
    job = scheduler.add_job(task_function, trigger, id=task_id)  # 添加任务
    task_map[task_id] = {
        'cron_expression': cron_expression,
        'task_function': task_function,
        'job': job
    }
    print(f"task {task_id} add success，cron task：{cron_expression}")
    return task_id

# 动态修改定时任务
def modify_task(task_id, new_cron_expression):
    if task_id in task_map:
        job = task_map[task_id]['job']
        job.remove()  # 删除原任务
        new_trigger = CronTrigger.from_crontab(new_cron_expression)  # 创建新触发器
        job = scheduler.add_job(task_map[task_id]['task_function'], new_trigger, id=task_id)  # 添加新任务
        task_map[task_id]['cron_expression'] = new_cron_expression  # 更新任务的 cron 表达式
        task_map[task_id]['job'] = job
        print(f"task {task_id} update success，new cron task：{new_cron_expression}")
    else:
        print(f"task {task_id} not exist，not update。")

# 动态删除定时任务
def delete_task(task_id):
    if task_id in task_map:
        job = task_map[task_id]['job']
        job.remove()  # 删除任务
        del task_map[task_id]  # 从任务映射中删除任务
        print(f"task {task_id} delete success")
    else:
        print(f"task {task_id} not exist，not delete。")


def time_to_cron(time_str):
    """
    将时间字符串（格式为 hh:mm:ss）转换为 CRON 表达式，以便每天在指定时间运行。

    参数:
    time_str (str): 时间字符串，格式为 "hh:mm:ss"。

    返回:
    str: 生成的 CRON 表达式。
    """
    # 分割时间字符串
    hours, minutes, seconds = map(int, time_str.split(':'))

    # 构建 CRON 表达式
    # 注意：CRON 表达式没有秒字段，所以我们忽略 seconds
    # 日期和星期几字段在这里我们设置为 '*' 和 '?'，表示每天（不指定星期几）
    cron_expression = f"{minutes} {hours} * * *"
    return cron_expression

def start_scheduler():
    scheduler.start()

    # 模拟主线程持续运行
    try:
        while True:
            time.sleep(1)
    except (KeyboardInterrupt, SystemExit):
        # 停止调度器
        scheduler.shutdown()




