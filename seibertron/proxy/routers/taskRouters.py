from flask import Blueprint, request
from service import taskService
from common import result

task_bp = Blueprint('task', __name__)


@task_bp.route('/set_camera_task', methods=['POST'])
def set_camera_task():
    task_config = request.get_json()
    print(task_config)
    return result.success(taskService.set_camera_task(task_config))


@task_bp.route('/get_camera_task', methods=['GET'])
def get_camera_task():

    return result.success(taskService.get_camera_task())