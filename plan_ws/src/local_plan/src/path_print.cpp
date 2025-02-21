#include <ros/ros.h>
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseStamped.h>

#include <vector>

std::vector<geometry_msgs::PoseStamped> local_points; // 存储提取的路径点

void pathCallback(const nav_msgs::Path::ConstPtr& msg) {
    // 清空原来的路径点
    local_points.clear();
    
    // 从消息中获取路径点
    const auto& poses = msg->poses;
    int size = poses.size();
    
    // 每隔5个点取一个，并确保取到最后一个点
    for (int i = 0; i < size; i += 5) {
        local_points.push_back(poses[i]);
    }
    
    // 如果最后一个点没有被取到，手动添加
    if (size > 0 && (size - 1) % 5 != 0) {
        local_points.push_back(poses[size - 1]);
    }

    // 打印取到的点
    ROS_INFO("local_points size: %zu", local_points.size());
    for (const auto& point : local_points) {
        ROS_INFO("Position: [%.2f, %.2f, %.2f]", point.pose.position.x, point.pose.position.y, point.pose.position.z);
    }
}

int main(int argc, char** argv)
{
    // 初始化ROS节点
    ros::init(argc, argv, "path_listener");
    ros::NodeHandle nh;

    // 订阅Path话题（假设话题名为 "path"）
    ros::Subscriber path_sub = nh.subscribe("/map_update_with_teb/teb_planner/local_plan", 1000, pathCallback);

    // 进入ROS事件循环
    ros::spin();

    return 0;
}