
from flask import Blueprint, request, Response, current_app

import time
import datetime
sse_bp = Blueprint('sse', __name__)


@sse_bp.route('/sse')
def sse():


    # 设置响应头以指示这是一个 SSE 连接
    headers = {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    }

    # 返回 Response 对象，其中包含生成事件的可迭代对象
    return Response(generate_events(), headers=headers)

def generate_events():
    # 发送一个初始的 "ping" 事件
    yield "event: ping\n"
    yield "data: Hello, world!\n\n"

    # 每隔5秒发送一个时间更新事件
    while True:
        if sse_result != None:
            yield "event: sse\n"
            yield f"data: {sse_result}\n"
            sse_result = None
        time.sleep(1)