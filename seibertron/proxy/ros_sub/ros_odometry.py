#!/usr/bin/env python3

import rospy
import json
from nav_msgs.msg import Odometry
import threading
import time
from server import webSocketServer

local_address = None
sub_topic = '/odom'

odometry_instance = None

class RosOdometry():
    def __init__(self):
        rospy.init_node('livox_lidar_listener', anonymous=True, disable_signals=True)

        # 创建一个Subscriber来订阅话题
        self.subcriber = rospy.Subscriber("/oula_odom", Odometry, self.callback)
        self.rate = rospy.Rate(1)
        print('订阅成功')

    def callback(self, data):
        x = data.pose.pose.position.x
        y = data.pose.pose.position.y
        z = data.pose.pose.position.z
        a = data.pose.pose.orientation.x
        b = data.pose.pose.orientation.y
        c = data.pose.pose.orientation.z
        w = data.pose.pose.orientation.w
        rospy.loginfo(f"Odometry Postion: x={x}, y={y}, z={z}, a={a}, b={b}, c={c}, w={w}")

        global local_address
        local_address = {
            'time': time.time(),
            'x': round(x,2),
            'y': round(y,2),
            'z': round(z,2),
            'a': a,
            'b': b,
            'c': c,
            'w': w,
            'tag': 'normal',
            'type': 'odom'
        }

        print(json.dumps(local_address))
        webSocketServer.sync_send_message_to_server(json.dumps(local_address))
        # global processes
        # if processes.is_set():
        #    rospy.loginfo(f"Odometry Postion: x={x}, y={y}, z={z}")
        #    data = {
        #        'x': x,
        #        'y': y,
        #        'tag': 'normal'
        #    }
        #    processes.clear()
    def ubsubscribe(self):
        self.subcriber.unregister()
        rospy.loginfo("unsubscribe from topic: /livox/lidar")
    
    def run(self):
        while not rospy.is_shutdown() and odometry_instance != None:
            self.rate.sleep()

def stop():
    global odometry_instance
    odometry_instance.ubsubscribe()
    odometry_instance = None

def start():
    if odometry_instance == None:
        # 初始化节点
        odometry_thread = threading.Thread(target=listener)
        odometry_thread.daemon = True
        odometry_thread.start()

def listener():
    global odometry_instance
    odometry_instance = RosOdometry()
    odometry_instance.run()
