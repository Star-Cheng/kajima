
from flask import Blueprint, request
from service import authService
from common.auth_decorator import require_permission
import token

auth_bp = Blueprint('auth', __name__)

@auth_bp.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.get_json()['username']
        password = request.get_json()['password']
        token = authService.login(username, password)

    return {'code': 200, 'status': 'success', 'message': '登录成功', 'data': {'token': token.decode('utf-8')}}

@auth_bp.route('/logout', methods=['GET'])
@require_permission("")
def logout():
    return {'code': 200, 'status': 'success', 'message': '退出成功！', 'data': True}
