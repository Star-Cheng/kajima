
def success(data):
    return {'code': 200, 'status': 'success', 'message': '成功', 'data': data }


def error(data):
    return {'code': 400, 'status': 'error', 'message': '失败', 'data': data }

def errorLogin():
    return {'code': 403, 'status': 'error', 'message': '登录失败！'}
