
from flask import Blueprint, request, session
from service import userService
from common import result

user_bp = Blueprint('user', __name__)


@user_bp.route('/list', methods=['GET'])
def user_list():
    access_token = userService.userList()
    return result.success(access_token)

@user_bp.route('/insert', methods=['POST'])
def user_insert():
    user = request.get_json()
    user = userService.user_inster(user)
    return result.success(user)

@user_bp.route('/update', methods=['PUT'])
def user_update():
    user = request.get_json()
    user = userService.user_update(user)
    return result.success(user)

@user_bp.route('/delete', methods=['DELETE'])
def user_delete():
    id = request.args.get("id")
    delete_result = userService.user_delete(id)
    if delete_result:
        return result.success(delete_result)
    else:
        return result.error(delete_result)
