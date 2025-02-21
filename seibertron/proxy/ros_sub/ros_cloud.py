#!/usr/bin/env python
import rospy
import threading
from sensor_msgs.msg import PointCloud2  # 确保这里导入了正确的消息类型
from server import webSocketServer
import json
import struct
import numpy as np
import sensor_msgs.point_cloud2 as pc2
import open3d as o3d
import time

stop_event = threading.Event()  # 用于控制停止的线程事件

cloud_instance = None

# 数据类型映射
datatype_to_struct = {
    pc2.PointField.FLOAT32: 'f',
    pc2.PointField.FLOAT64: 'd',
    pc2.PointField.INT32: 'i',
    pc2.PointField.UINT32: 'I',
    pc2.PointField.INT16: 'h',
    pc2.PointField.UINT16: 'H',
    pc2.PointField.INT8: 'b',
    pc2.PointField.UINT8: 'B',
    # 如果需要，可以添加更多类型
}
 

class RosCloud:
    def __init__(self):
        rospy.init_node('livox_lidar_listener', anonymous=True, disable_signals=True)

        # 创建一个Subscriber来订阅话题
        self.subcriber = rospy.Subscriber("/cloud_registered", PointCloud2, self.callback)
        self.rate = rospy.Rate(1)
        self.lasttime = 0

        print('订阅成功')

    def callback(self, msg):
        """

        当接收到/livox/lidar话题上的数据时，此回调函数将被调用。

        :param data: 接收到的CustomMsg类型的数据。

        """

        rospy.loginfo("Received data from /cloud_registered")
        
        if time.time() - self.lasttime > 1:
            self.lasttime = time.time()
        else:
            return
        # 获取点的字段信息
        fields = msg.fields
        
        # 查找 x, y, z 字段的索引
        x_idx = None
        y_idx = None
        z_idx = None
        for i, field in enumerate(fields):
            if field.name == 'x':
                x_idx = i
            elif field.name == 'y':
                y_idx = i
            elif field.name == 'z':
                z_idx = i
        
        if x_idx is None or y_idx is None or z_idx is None:
            raise ValueError("Point cloud does not contain 'x', 'y', or 'z' fields")
        
        # 计算每个点所占用的字节数（即一个完整点的字节大小）
        point_step = 0
        for field in fields:
            if field.datatype not in datatype_to_struct:
                raise ValueError(f"Unsupported datatype for field {field.name}: {field.datatype}")
            datatype_str = datatype_to_struct[field.datatype]
            point_step += field.count * struct.calcsize(datatype_str)
        
        # 解析点数据
        points = []
        lidar = {'id': msg.header.seq, 'point_num': msg.width, 'type': 'lidar'}

        data = np.frombuffer(msg.data, dtype=np.uint8)
        for i in range(0, msg.row_step, msg.point_step):
            # 计算每个字段的偏移量（这里假设字段是紧密打包的）
            x_offset = x_idx * struct.calcsize(datatype_to_struct[fields[x_idx].datatype]) * fields[x_idx].count
            y_offset = y_idx * struct.calcsize(datatype_to_struct[fields[y_idx].datatype]) * fields[y_idx].count
            z_offset = z_idx * struct.calcsize(datatype_to_struct[fields[z_idx].datatype]) * fields[z_idx].count
            
            # 注意：这里我们假设每个字段只包含一个值（即 count=1），并且没有额外的填充
            # 如果字段包含多个值（例如向量），则需要相应地调整偏移量和解析逻辑
            
            # 解析 x, y, z 值（这里假设数据类型为 FLOAT32）
            x = struct.unpack_from(f'{datatype_to_struct[fields[x_idx].datatype]}', data[i + x_offset:i + x_offset + struct.calcsize(datatype_to_struct[fields[x_idx].datatype])])[0]
            y = struct.unpack_from(f'{datatype_to_struct[fields[y_idx].datatype]}', data[i + y_offset:i + y_offset + struct.calcsize(datatype_to_struct[fields[y_idx].datatype])])[0]
            z = struct.unpack_from(f'{datatype_to_struct[fields[z_idx].datatype]}', data[i + z_offset:i + z_offset + struct.calcsize(datatype_to_struct[fields[z_idx].datatype])])[0]
            
            points.append([round(x, 3), round(y, 3), round(z, 3)])
        # 在这里处理接收到的数据
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(points)
        # 设置体素大小进行降采样
        voxel_size = 0.1 # 例如，0.05 米
        # 进行降采样
        down_pcd = pcd.voxel_down_sample(voxel_size=voxel_size)

        points_dict_list = []
        for point in down_pcd.points:
            points_dict_list.append([round(point[0], 3), round(point[1], 3), round(point[2], 3)])
        lidar['point_num'] = len(points_dict_list)
        lidar['points'] = points_dict_list
        print(json.dumps(lidar))
        # 例如，打印点云数据的某些属性
        webSocketServer.sync_send_message_to_server(json.dumps(lidar))  
    
        
        
    def ubsubscribe(self):
        self.subcriber.unregister()
        rospy.loginfo("unsubscribe from topic: /cloud_registered")
    
    def run(self):
        while not rospy.is_shutdown() and cloud_instance != None:
            self.rate.sleep()

def stop():
    global cloud_instance
    cloud_instance.ubsubscribe()
    cloud_instance = None

def start():
    if cloud_instance == None:
        # 初始化节点
        cloud_thread = threading.Thread(target=listener)
        cloud_thread.daemon = True
        cloud_thread.start()

def listener():
    global cloud_instance
    cloud_instance = RosCloud()
    cloud_instance.run()


if __name__ == '__main__':
    listener()