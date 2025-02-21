from db import authdb, userdb
from utils.jwtTokenUtil import  JWTTool

def login(username, password):
    user = authdb.load_user(username)

    if user is None:
        return False
    if user['password'] != password:
        return False

    jwt_tool = JWTTool()
    user['password'] = None
    # 生成一个包含用户 ID 和过期时间的 JWT 令牌（1 小时后过期）
    token = jwt_tool.generate_token(user)
    return token

def logout():
    print("Logging out...")


def loadByUsername(username):
    print("Loading user...")
