#include <ros/ros.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Twist.h>

int main(int argc, char** argv) {
    ros::init(argc, argv, "odometry_publisher");
    ros::NodeHandle nh;

    // 创建一个发布者，发布到 /odom 话题
    ros::Publisher odom_pub = nh.advertise<nav_msgs::Odometry>("/odom", 10);

    // 设置循环频率
    ros::Rate rate(1); // 10 Hz

    // int i=0;

    while (ros::ok()) {
        // 创建 Odometry 消息
        nav_msgs::Odometry odom_msg;

        // 设置时间戳
        odom_msg.header.stamp = ros::Time::now();
        odom_msg.header.frame_id = "/map_update_with_teb/teb_planner/odom";

        // 设置位置 (5, 5, 0)
        odom_msg.pose.pose.position.x = 0.0;
        odom_msg.pose.pose.position.y = 0.0;
        odom_msg.pose.pose.position.z = 0.0;

        // 设置默认旋转（没有旋转）
        odom_msg.pose.pose.orientation.w = 1.0;


        // if (i>9) i=0;

        // odom_msg.pose.pose.position.x = i;
        // odom_msg.pose.pose.position.y = 0.0;
        // odom_msg.pose.pose.position.z = 0.0;

        // i++;        

        // 发布消息
        odom_pub.publish(odom_msg);

        // 打印信息（可选）
        ROS_INFO("Published Odometry: x=%f, y=%f", odom_msg.pose.pose.position.x, odom_msg.pose.pose.position.y);

        // 按照设定频率休眠
        rate.sleep();
    }

    return 0;
}