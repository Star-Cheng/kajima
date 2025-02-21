import jwt
import datetime
from typing import Dict, Any

class JWTTool:
    def __init__(self, algorithm: str = 'HS256'):
        """
        初始化 JWT 工具类

        :param secret_key: 用于签名 JWT 的秘密密钥
        :param algorithm: 用于签名 JWT 的算法（默认为 HS256）
        """
        self.secret_key = 'test'
        self.algorithm = algorithm

    def generate_token(self, payload: Dict[str, Any]) -> str:
        """
        生成 JWT 令牌

        :param payload: 要包含在 JWT 中的数据
        :param expires_delta: 令牌过期时间（相对于当前时间的增量），默认为 None（不设置过期时间）
        :return: 生成的 JWT 令牌
        """
        expires_delta = datetime.timedelta(hours=1)
        if expires_delta:
            expiration = datetime.datetime.utcnow() + expires_delta
        else:
            expiration = None

        # 将过期时间添加到 payload 中（如果需要）
        if expiration:
            payload['exp'] = expiration

        # 生成 JWT 令牌
        token = jwt.encode(payload, self.secret_key, algorithm=self.algorithm)
        return token

    def verify_token(self, token: str, claims_to_check: Dict[str, Any] = None) -> Dict[str, Any]:
        """
        验证 JWT 令牌

        :param token: 要验证的 JWT 令牌
        :param claims_to_check: 要检查的声明（可选），字典形式，键为要检查的声明名，值为期望的值
        :return: 如果令牌有效，返回 payload；否则引发 jwt.ExpiredSignatureError 或 jwt.InvalidTokenError 异常
        """
        try:
            # 验证 JWT 令牌并获取 payload
            payload = jwt.decode(token, self.secret_key, algorithms=[self.algorithm], options={'verify_exp': True})

            # 检查特定的声明（如果需要）
            if claims_to_check:
                for key, value in claims_to_check.items():
                    if key not in payload or payload[key] != value:
                        raise jwt.InvalidTokenError(f"Invalid claim '{key}': {payload.get(key)}")

            return payload
        except jwt.ExpiredSignatureError:
            raise jwt.ExpiredSignatureError("Token has expired")
        except jwt.InvalidTokenError as e:
            raise jwt.InvalidTokenError(f"Invalid token: {e}")


# 示例用法
if __name__ == "__main__":

    # 初始化 JWT 工具类
    jwt_tool = JWTTool()

    # 生成一个包含用户 ID 和过期时间的 JWT 令牌（1 小时后过期）
    payload = {'user_id': 123, 'username': 'john_doe'}
    expires_delta = datetime.timedelta(hours=1)
    token = jwt_tool.generate_token(payload)
    print(f"Generated Token: {token}")

    # 验证 JWT 令牌
    try:
        decoded_payload = jwt_tool.verify_token(token)
        print(f"Verified Payload: {decoded_payload}")
    except jwt.ExpiredSignatureError as e:
        print(f"Token verification failed: {e}")
    except jwt.InvalidTokenError as e:
        print(f"Token verification failed: {e}")