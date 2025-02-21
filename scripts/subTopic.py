#!/usr/bin/env python3
 
import rospy
import json
from nav_msgs.msg import Odometry  # 假设我们订阅的是一个String类型的话题
import threading


processes = threading.Event()

def callback(data):
    x = data.pose.pose.position.x
    y = data.pose.pose.position.y
    z = data.pose.pose.position.z
    rospy.loginfo(f"Odometry Postion: x={x}, y={y}, z={z}")

    global processes

    if processes.is_set():
        rospy.loginfo(f"Odometry Postion: x={x}, y={y}, z={z}")
        with open('odometry.txt', 'a') as f:
            f.write(json.dumps({
                                    'x': x,
                                    'y': y,
                                    'z': z
                                }) + '\r')
        processes.clear()


def start():
        # 初始化节点
    rospy.init_node('listener', anonymous=True)
    
    # 创建一个Subscriber来订阅话题
    rospy.Subscriber("/Odometry", Odometry, callback)
    print('订阅成功')
    thread = threading.Thread(target=listener)
    thread.start()

def listener():
    # 进入消息循环
    rospy.spin()
    
 
if __name__ == '__main__':
    listener()