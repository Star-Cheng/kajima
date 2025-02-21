import jwt
from flask import request
from utils.jwtTokenUtil import JWTTool
from common import result

def require_permission(permission):
    def decorator(func):
        def wrapper(*args, **kwargs):
            if check_permission(permission):
                return func(*args, **kwargs)
            else:
                return result.errorLogin()

        return wrapper

    return decorator


# 检查权限的辅助函数
def check_permission(permission):
    auth_header = request.headers.get("Authorization")
    jwt_tool = JWTTool()
    # 检查请求头是否存在且以 Bearer 开头
    if auth_header and auth_header.startswith('Bearer '):
        # 截取 Bearer 后面的令牌部分
        token = auth_header.split(' ')[1]
        # 验证 JWT 令牌
        try:
            decoded_payload = jwt_tool.verify_token(token)
            print(f"Verified Payload: {decoded_payload}")
            return True
        except jwt.ExpiredSignatureError as e:
            print(f"Token verification failed: {e}")
            return False
        except jwt.InvalidTokenError as e:
            print(f"Token verification failed: {e}")
            return False

    return False