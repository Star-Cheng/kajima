#include <ros/ros.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>

int main(int argc, char** argv)
{
    ros::init(argc, argv, "static_tf_publisher");
    ros::NodeHandle nh;

    // 创建 TF 广播器
    tf2_ros::StaticTransformBroadcaster static_broadcaster;
    
    // 创建一个变换消息
    geometry_msgs::TransformStamped transformStamped;

    // 设置变换的参数
    transformStamped.header.stamp = ros::Time::now();
    transformStamped.header.frame_id = "map";  // 父坐标系
    transformStamped.child_frame_id = "base_link";  // 子坐标系
    transformStamped.transform.translation.x = 0.0;  // X 轴偏移
    transformStamped.transform.translation.y = 0.0;  // Y 轴偏移
    transformStamped.transform.translation.z = 0.0;  // Z 轴偏移
    transformStamped.transform.rotation.x = 0.0;  // X 轴旋转
    transformStamped.transform.rotation.y = 0.0;  // Y 轴旋转
    transformStamped.transform.rotation.z = 0.0;  // Z 轴旋转
    transformStamped.transform.rotation.w = 1.0;  // W 轴旋转

    // 发布变换
    static_broadcaster.sendTransform(transformStamped);

    ros::spin(); // 进入循环，保持节点运行

    return 0;
}