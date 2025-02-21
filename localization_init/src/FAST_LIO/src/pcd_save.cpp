#include "ros/ros.h"
#include "std_msgs/Bool.h"

void savepcd_Callback(const std_msgs::Bool::ConstPtr& msg) {
    if (msg->data) {
        ros::shutdown(); 
    }
}

int main(int argc, char **argv) {
    // 初始化ROS节点
    ros::init(argc, argv, "bool_publisher");

    // 创建节点句柄
    ros::NodeHandle nh;

    // 创建一个发布者，发布std_msgs/Bool消息，队列大小为10
    ros::Publisher bool_pub = nh.advertise<std_msgs::Bool>("/save_pcd", 10);
    ros::Subscriber pcd_save = nh.subscribe("/has_save_pcd", 10, savepcd_Callback);

    // 循环频率，例如：1Hz
    ros::Rate loop_rate(10);

    while (ros::ok()) {
        // 创建一个布尔消息
        std_msgs::Bool msg;
        msg.data = true; // 或者根据条件设置为false

        // 发布消息
        bool_pub.publish(msg);

        // 打印消息内容
        ROS_INFO("Publishing bool message: [%s]", msg.data ? "true" : "false");

        // 等待下一个循环周期
        ros::spinOnce();
        loop_rate.sleep();
    }

    return 0;
}