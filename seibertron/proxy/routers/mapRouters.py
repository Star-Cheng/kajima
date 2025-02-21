from flask import Blueprint, request, send_file
from service import mapService
from common import result

map_bp = Blueprint('map', __name__)

@map_bp.route('/list', methods=['GET'])
def map_list():
    # 序号 名称
    return result.success(mapService.map_list())

@map_bp.route('/get_map', methods=['GET'])
def get_map():
    index = request.args.get("id")
    # 序号 名称
    return send_file(mapService.get_map(index))


@map_bp.route('/dense_list', methods=['GET'])
def dense_list():
    index = request.args.get("id")
    print(index)
    return result.success(mapService.dense_list(index))

@map_bp.route('/start', methods=['GET'])
def start():
    mapService.map_start()
    return result.success(True)


@map_bp.route('/stop', methods=['GET'])
def stop():
    mapService.map_stop()
    return result.success(True)


@map_bp.route('/backup', methods=['GET'])
def backup_map():
    index = request.args.get("id")
    name = request.args.get("name")
    
    return result.success(mapService.map_backup(index))

@map_bp.route('/restore', methods=['GET'])
def restore_map():
    index = request.args.get("id")
    delete_result = mapService.map_change(index)
    if delete_result:
        return result.success(delete_result)
    else:
        return result.error(delete_result)
    

@map_bp.route('/delete', methods=['DELETE'])
def delete_map():
    index = request.args.get("id")
    delete_result = mapService.map_delete(index)
    if delete_result:
        return result.success(delete_result)
    else:
        return result.error(delete_result)
    