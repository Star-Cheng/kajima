#include <ros/ros.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <opencv2/opencv.hpp>
#include <sensor_msgs/image_encodings.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <teb_local_planner/teb_local_planner_ros.h>
#include <teb_local_planner/teb_config.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Point.h>
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <std_msgs/Bool.h>

cv::Mat image;
cv::Mat image1;

bool first = false;
bool new_image_received = false; // 新增标志位，表示是否接收到新图片

int fail_count=0;//规划失败计数
// nav_msgs::Odometry odom;

costmap_2d::Costmap2D* costmap;


geometry_msgs::Twist cmd_vel; // 用于存储速度信息

Eigen::Matrix<double, 3, 1> global_target; // 当前全局目的坐标点
Eigen::Matrix<double, 3, 1> final_goal; // 当前小车坐标系下全局目的坐标点
Eigen::Matrix<double, 3, 1> current_pose; // 小车当前位置
Eigen::Matrix<double, 3, 3> rotation; // 旋转矩阵

double current_yaw = 0; // 偏航角
double position_x = 0;
double position_y = 0;

void getOdom(const nav_msgs::Odometry::ConstPtr& odo_msg); // mid360定位姿态

void toEulerAngle(const Eigen::Quaternionf &q, double &roll, double &pitch, double &yaw);//四元数转欧拉角

//接受当前全局目标点
void globalpointCallback(const geometry_msgs::Point::ConstPtr& msg);

// 回调函数: 接收图像数据
void mapCallback(const sensor_msgs::Image::ConstPtr& msg);


// 主函数
int main(int argc, char** argv)
{
    ros::init(argc, argv, "map_update_with_teb");
    ros::NodeHandle nh;

    ros::Subscriber ODOMnewsub = nh.subscribe<nav_msgs::Odometry>("/oula_odom", 10, getOdom); // 获取定位信息
    ros::Subscriber point_sub = nh.subscribe("/teb_target_point", 1000, globalpointCallback);

    // 创建Publisher，发布到话题"/plan_fail"
    ros::Publisher fail_pub = nh.advertise<std_msgs::Bool>("/plan_fail", 10);
    std_msgs::Bool fail_msg;

    tf2_ros::Buffer tf_buffer;
    tf2_ros::TransformListener tf_listener(tf_buffer);

    costmap_2d::Costmap2DROS costmap_ros("local_costmap", tf_buffer);

    teb_local_planner::TebConfig tebConfig;
    tebConfig.loadRosParamFromNodeHandle(nh);
    // tebConfig.hcp.enable_homotopy_class_planning=false;

    teb_local_planner::TebLocalPlannerROS teb_planner;
    teb_planner.initialize("teb_planner", &tf_buffer, &costmap_ros);

    // ROS_INFO("%s",&costmap_ros);

    costmap_ros.start();
    costmap = costmap_ros.getCostmap();

    // teb_pri=&teb_planner;

    ros::Subscriber map_sub = nh.subscribe("/image_topic", 10, mapCallback);



    rotation << cos(current_yaw), sin(current_yaw), 0,
        (-sin(current_yaw)), cos(current_yaw), 0,
        0, 0, 1; // 旋转矩阵

    geometry_msgs::PoseStamped goal;
    goal.header.frame_id = "map";
    goal.pose.position.x = 10.0;
    goal.pose.position.y = 10.0;
    goal.pose.position.z = 0.0;
    goal.pose.orientation.w = 1.0;
    goal.pose.orientation.x = 0.0;
    goal.pose.orientation.y = 0.0;
    goal.pose.orientation.z = 0.0;

    while (!first)//等待图片数据
    {
        ros::spinOnce();
    }

    // ros::Rate rate(0.1); // 10Hz
    while (ros::ok())
    {
        ros::spinOnce();

        if (new_image_received) // 检测是否接收到新图片
        {
            // 更新目标点
            goal.pose.position.x = final_goal(0, 0);
            goal.pose.position.y = final_goal(1, 0);

            std::cout<<final_goal;

            // 设定路径计划目标
            if (teb_planner.setPlan({goal}))
                std::cout << "Set goal successfully" << std::endl;

            // 计算速度指令
            if (teb_planner.computeVelocityCommands(cmd_vel))
            {
                std::cout << "Velocity command obtained" << std::endl;
                fail_count=0;
                fail_msg.data = false;      
                fail_pub.publish(fail_msg);
            }
            else//失败计数
            {
                fail_count++;
                if(fail_count>3)
                {
                    fail_msg.data = true;      
                    fail_pub.publish(fail_msg);
                }
            }


            ROS_INFO("Velocity: linear: [%.2f, %.2f], angular: %.2f",
                     cmd_vel.linear.x, cmd_vel.linear.y, cmd_vel.angular.z);

            // 重置标志位，等待下一张图片
            new_image_received = false;
        }

        // ros::spinOnce();
        // rate.sleep();
    }

    return 0;
}

// 回调函数: 接收图像数据
void mapCallback(const sensor_msgs::Image::ConstPtr& msg)
{

    if (!first) first = true;

    image1.release();
    image.release();

    image1 = cv_bridge::toCvShare(msg, "bgr8")->image;
    cv::imwrite("costmap.jpg", image1);

    image = cv::imread("costmap.jpg", cv::IMREAD_GRAYSCALE);

    cv::flip(image, image, 0);

    unsigned int width = image.cols;
    unsigned int height = image.rows;

    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            // unsigned char cost = (image.at<uchar>(y, x) == 255) ?
            //     costmap_2d::FREE_SPACE : costmap_2d::LETHAL_OBSTACLE;

            unsigned char cost = (image.at<uchar>(y, x) >= 200) ?
                costmap_2d::FREE_SPACE : costmap_2d::LETHAL_OBSTACLE;

            if (x < costmap->getSizeInCellsX() && y < costmap->getSizeInCellsY())
            {
                costmap->setCost(x, y, cost);
            }

            // costmap->setCost(x, y, costmap_2d::FREE_SPACE);
        }
    }

    // costmap->updateMap();

    // 标志位设为true，表示接收到新图片
    new_image_received = true;

    ROS_INFO("Costmap updated successfully.");

    // if(teb_pri->computeVelocityCommands(cmd_vel)) std::cout<<"get_velocity"<<std::endl;
}


void getOdom(const nav_msgs::Odometry::ConstPtr& odo_msg) // mid360定位姿态
{
    // if(!start_falg) start_falg=1;
    position_x = odo_msg->pose.pose.position.x;
    position_y = odo_msg->pose.pose.position.y;
    // current_yaw = odo_msg->pose.pose.orientation.z; // YAW角

    Eigen::Quaternionf quaternion(odo_msg->pose.pose.orientation.w,
                                odo_msg->pose.pose.orientation.x,
                                odo_msg->pose.pose.orientation.y,
                                odo_msg->pose.pose.orientation.z);

    double roll, pitch, yaw;
    toEulerAngle(quaternion, roll, pitch, yaw);
    current_yaw =yaw;
    current_pose << position_x, position_y, 0;
}

void toEulerAngle(const Eigen::Quaternionf &q, double &roll, double &pitch, double &yaw)//四元数转欧拉角
{
    // roll (x-axis rotation)
    double sinr_cosp = +2.0 * (q.w() * q.x() + q.y() * q.z());
    double cosr_cosp = +1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y());
    roll = atan2(sinr_cosp, cosr_cosp) * 180 / M_PI;

    // pitch (y-axis rotation)
    double sinp = +2.0 * (q.w() * q.y() - q.z() * q.x());
    if (fabs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp) * 180 / M_PI; //
    else
        pitch = asin(sinp) * 180 / M_PI;

    // yaw (z-axis rotation)
    double siny_cosp = +2.0 * (q.w() * q.z() + q.x() * q.y());
    double cosy_cosp = +1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z());
    yaw = atan2(siny_cosp, cosy_cosp) * 180 / M_PI;
}

//接受当前全局目标点
void globalpointCallback(const geometry_msgs::Point::ConstPtr& msg)
{
    global_target(0,0)=msg->x;
    global_target(1,0)=msg->y;
    global_target(2,0)=0;    

    // std::cout<<"global target"<<global_target<<std::endl;

    rotation << cos(current_yaw/ 180 * M_PI), sin(current_yaw/ 180 * M_PI), 0,
        (-sin(current_yaw/ 180 * M_PI)), cos(current_yaw/ 180 * M_PI), 0,
        0, 0, 1; // 更新旋转矩阵

    final_goal = rotation*(global_target - current_pose);//计算小车坐标系下全局目标点
    // std::cout<<"final_goal"<<final_goal(0,0)<<"          "<<final_goal(1,0)<<std::endl;
    // std::cout<<"current_pose"<<current_pose(0,0)<<"          "<<current_pose(1,0)<<std::endl;
    // ROS_INFO("Received Point: x = [%f], y = [%f], z = [%f]", msg->x, msg->y, msg->z);
}