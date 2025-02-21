import json
from flask import Blueprint, request
from service import pathService
from common import result

path_bp = Blueprint('path', __name__)

@path_bp.route('/list', methods=['GET'])
def path_list():
    number = request.args.get("id")

    data = pathService.path_load(number)
    return result.success(data)


@path_bp.route('/start_plan', methods=['GET'])
def start_plan():
    pathService.plan_start()
    return result.success(True)

@path_bp.route('/stop_plan', methods=['GET'])
def stop_plan():
    pathService.plan_stop()
    return result.success(True)

@path_bp.route('/save', methods=['POST'])
def path_save():
    number = request.get_json()['id']
    data = request.get_json()['path']
    pathService.path_save(number, data)
    return result.success(True)

@path_bp.route('/backup', methods=['POST'])
def path_backup():
    data = request.get_json()
    map_data = pathService.backup_path(data)
    return result.success(map_data)

@path_bp.route('/restore', methods=['GET'])
def restore_path():
    name = request.args.get("name")
    map_id = request.args.get("map_id")
    restore_result = pathService.path_restore(name, map_id)
    if restore_result:
        return result.success(restore_result)
    else:
        return result.error(restore_result)
    
@path_bp.route('/backup_list', methods=['GET'])
def path_backup_lists():
    number = request.args.get("id")
    data = pathService.backup_list(number)
    return result.success(data)

@path_bp.route('/start_nav', methods=['GET'])
def path_start_navigation():
    pathService.nav_start()
    return result.success(True)


@path_bp.route('/stop_nav', methods=['GET'])
def path_stop_navigation():
    pathService.nav_stop()
    return result.success(True)