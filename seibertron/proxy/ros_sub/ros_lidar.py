#!/usr/bin/env python
import rospy
import threading
from livox_ros_driver2.msg import CustomMsg  # 确保这里导入了正确的消息类型
from server import webSocketServer
import json
import open3d as o3d
import time

stop_event = threading.Event()  # 用于控制停止的线程事件

lidar_instance = None


class RosLidar:
    def __init__(self):
        rospy.init_node('livox_lidar_listener', anonymous=True, disable_signals=True)

        # 创建一个Subscriber来订阅话题
        self.subcriber = rospy.Subscriber("/livox/lidar", CustomMsg, self.callback)
        self.lasttime = 0
        self.point_array = []
        print('订阅成功')

    def callback(self, data):
        """

        当接收到/livox/lidar话题上的数据时，此回调函数将被调用。

        :param data: 接收到的CustomMsg类型的数据。

        """
        if time.time() - self.lasttime > 1:
            self.lasttime = time.time()
        else:
            return
        rospy.loginfo("Received data from /livox/lidar")

        lidar = {'id': data.lidar_id, 'point_num': data.point_num, 'type': 'lidar'}
        point_array = []
        for point in data.points:
            point_array.append([round(point.x, 3), round(point.y, 3), round(point.z, 3)])
        # 在这里处理接收到的数据
        pcd = o3d.geometry.PointCloud()
        pcd.points = o3d.utility.Vector3dVector(point_array)
        # 设置体素大小进行降采样
        voxel_size = 1 # 例如，0.05 米
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
        #

    def ubsubscribe(self):
        self.subcriber.unregister()
        rospy.loginfo("unsubscribe from topic: /livox/lidar")
    
    def run(self):
        rate = rospy.Rate(1)
        while not rospy.is_shutdown() and lidar_instance != None:
            rate.sleep()

def stop():
    global lidar_instance
    lidar_instance.ubsubscribe()
    lidar_instance = None

def start():
    if lidar_instance == None:
        # 初始化节点
        lidar_thread = threading.Thread(target=listener)
        lidar_thread.daemon = True
        lidar_thread.start()

def listener():
    global lidar_instance
    lidar_instance = RosLidar()
    lidar_instance.run()


if __name__ == '__main__':
    listener()