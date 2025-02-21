import websockets
import asyncio
import json
import threading

lock = threading.Lock()

server_instance = None  # ���ڱ��� WebSocket ��������ȫ��ʵ��

class WebSocketServer:
    def __init__(self, host="0.0.0.0", port=8765):
        self.host = host  # ��������������ַ
        self.port = port  # �������Ķ˿ں�
        self.connected_clients = set()  # �����������ӵĿͻ���
        self.message_queue = asyncio.Queue()  # ��Ϣ���У����ڴ洢���㲥����Ϣ
        self.server_task = None  # ���������񣬴洢���������첽����
        self.server = None  # ����������

    async def websocket_handler(self, websocket, path):
        # �����ͻ������ӵ�Э��
        self.connected_clients.add(websocket)  # ���ӿͻ��˵������ӿͻ��˼���
        try:
            async for message in websocket:  # ���տͻ��˵���Ϣ
                await websocket.send(f"Echo: {message}")  # ������Ϣ���ͻ���
        except websockets.ConnectionClosed:
            print("Connection closed")  # ���ӹر�ʱ�Ĵ���
        finally:
            self.connected_clients.remove(websocket)  # ���ͻ��˴������ӿͻ��˼����Ƴ�

    async def broadcast_message(self, message):
        # �����������ӵĿͻ��˹㲥��Ϣ
        if self.connected_clients:
            await asyncio.gather(*(client.send(message) for client in self.connected_clients))

    async def process_message_queue(self):
        # ������Ϣ�����е���Ϣ���㲥
        while True:
            message = await self.message_queue.get()
            await self.broadcast_message(message)

    async def start(self):
        # ���� WebSocket ������
        self.server = await websockets.serve(self.websocket_handler, self.host, self.port)
        self.server_task = asyncio.create_task(self.server.wait_closed())
        print(f"WebSocket server running on ws://{self.host}:{self.port}")
        await self.process_message_queue()  

    async def send_message(self, message):
        lock.acquire()
        await self.broadcast_message(message)
        lock.release()
    async def stop(self):
        
        for client in self.connected_clients:
            await client.close()  
        
        if self.server:
            self.server.close()
            await self.server.wait_closed()
        if self.server_task:
            self.server_task.cancel()

async def start_server():
    
    global server_instance
    server_instance = WebSocketServer()  
    await server_instance.start()  

async def send_message_to_server(message):
    loop = asyncio.get_running_loop()
    global server_instance
    if server_instance:
        await server_instance.send_message(message)  
    else:
        print("Server is not running or event loop is not active.")

def sync_send_message_to_server(message):
    
    asyncio.run(send_message_to_server(message))
