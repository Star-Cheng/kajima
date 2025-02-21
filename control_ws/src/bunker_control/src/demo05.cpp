#include <ros/ros.h>
#include <std_msgs/String.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <nav_msgs/Odometry.h>
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <geometry_msgs/TwistStamped.h>
#include <cmath>
#include <std_msgs/Bool.h>

using json = nlohmann::json;

// 定义一个结构体来存储目标点
struct Point {
    double x;
    double y;
    std::string tag;  // 新增tag字段，用于区分普通航迹点和任务点
};

struct PIDParams {
    std::string name;
    double P;
    double I;
    double D;
};

double current_yaw = 0; // 偏航角
double position_x = 0;
double position_y = 0;

int target_id = 0;

double error_distance = 0;
double error_angle = 0;

double linear_x = 0;
double angular_z = 0;

double target_yaw = 0;

bool tasking=0;//执行任务
bool start_falg=0;//开始标记

bool stop_flag=0;//停障信号

double limit_x=0.8;//限积分
double limit_z=0.15;

geometry_msgs::Twist cmd_speed;

class PID {
public:
    PID(double kp, double ki, double kd)
        : kp_(kp), ki_(ki), kd_(kd), prev_error_(0), integral_(0) {}

    double calculate(double error, double limit) {
        integral_ += error;
        double derivative = error - prev_error_;
        if (integral_ > limit) integral_ = limit;
        if (integral_ < -limit) integral_ = -limit;
        prev_error_ = error;
        return kp_ * error + ki_ * integral_ + kd_ * derivative;
    }

    void clear_integral()
    {
        integral_=0;
    }

private:
    double kp_;
    double ki_;
    double kd_;
    double prev_error_;
    double integral_;
};

void getOdom(const nav_msgs::Odometry::ConstPtr& odo_msg); // mid360定位姿态

void get_cargoal(const std_msgs::Bool::ConstPtr& task_msg); // /car_goal

void get_stop(const std_msgs::Bool::ConstPtr& stop_msg); // /car_goal获取/stop_signl停障信息

// 读取JSON文件并解析目标点数据
std::vector<Point> readJsonFile(const std::string& filename);
// 读取JSON文件并解析PID参数
bool readPIDParams(const std::string& filename, PIDParams& controller1, PIDParams& controller2);

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");
    ros::init(argc, argv, "control_node");
    ros::NodeHandle nh;
    ros::Subscriber ODOMnewsub = nh.subscribe<nav_msgs::Odometry>("/oula_odom", 10, getOdom); // 获取定位信息
    ros::Publisher pub = nh.advertise<geometry_msgs::Twist>("/smoother_cmd_vel", 10); // 发布控制信息
    ros::Subscriber car_goalsub = nh.subscribe<std_msgs::Bool>("/car_goal", 10, get_cargoal); // 获取/car_goal信息
    ros::Subscriber stopsub = nh.subscribe<std_msgs::Bool>("/stop_signl", 10, get_stop); // 获取/stop_signl停障信息
    ros::Rate loop_rate(50);
    PIDParams controller_x;
    PIDParams controller_z;

    if (readPIDParams("/home/agile/control_ws/src/bunker_control/src/pid_params.json", controller_x, controller_z)) {
        // 输出读取到的PID参数
        std::cout << controller_x.name << std::endl;
        std::cout << "P: " << controller_x.P << std::endl;
        std::cout << "I: " << controller_x.I << std::endl;
        std::cout << "D: " << controller_x.D << std::endl;

        std::cout << controller_z.name << std::endl;
        std::cout << "P: " << controller_z.P << std::endl;
        std::cout << "I: " << controller_z.I << std::endl;
        std::cout << "D: " << controller_z.D << std::endl;
    } else {
        std::cerr << "Failed to read PID parameters from JSON file" << std::endl;
        return 1;
    }

    std::vector<Point> points;
    

    try {
        // 读取JSON文件并获取目标点
        std::string filename = "/home/agile/control_ws/src/bunker_control/src/target.json";
        points = readJsonFile(filename);
    } catch (const std::exception& e) {
        ROS_ERROR("Error: %s", e.what());
    }

    Eigen::Matrix<double, 3, 1> target[points.size()]; // 目标点

    for (int i = 0; i < points.size(); i++) // 载入目标点
    {
        target[i] << points[i].x, points[i].y, 0;
    }

    ROS_INFO("载入目标点成功");
    ROS_INFO("%ld", points.size());

    Eigen::Matrix<double, 3, 3> rotation; // 旋转矩阵

    rotation << cos(current_yaw), sin(current_yaw), 0,
        (-sin(current_yaw)), cos(current_yaw), 0,
        0, 0, 1; // 旋转矩阵

    Eigen::Matrix<double, 3, 1> current_pose; // 当前位置
    current_pose << position_x, position_y, 0;

    Eigen::Matrix<double, 3, 1> final_target; // 当前目的坐标点

    PID pid_linear(controller_x.P, controller_x.I, controller_x.D); // PID参数需要根据实际情况调整
    PID pid_angular(controller_z.P, controller_z.I, controller_z.D);

    ROS_INFO("初始化完成");
    while(!start_falg)
    {
        ros::spinOnce(); // 检查里程计消息       
        ROS_INFO("等待中");
        ros::Duration(1.0).sleep(); // 等待1秒
    }

    while (ros::ok()) {
        ros::spinOnce(); // 检查里程计消息

        while (stop_flag)// 停障
        {
            ROS_INFO("停障");
            ros::spinOnce(); // 检查里程计消息
            cmd_speed.linear.x = 0;       
            cmd_speed.angular.z = 0;
            pub.publish(cmd_speed); // 发布控制信息
            ros::Duration(1.0).sleep(); // 等待1秒
        }
        

        // doubule current_angle=current_yaw
        rotation << cos(current_yaw/ 180 * M_PI), sin(current_yaw/ 180 * M_PI), 0,
            (-sin(current_yaw/ 180 * M_PI)), cos(current_yaw/ 180 * M_PI), 0,
            0, 0, 1; // 更新旋转矩阵
        // std::cout<<"current angle"<<current_yaw<<std::endl;
        // std::cout<<"cos"<<cos(current_yaw/ 180 * M_PI)<<std::endl;
        // std::cout<<rotation<<std::endl;

        current_pose << position_x, position_y, 0; // 更新位置

        // final_target = (target[target_id] - current_pose);
        final_target = rotation*(target[target_id] - current_pose);
        // std::cout<<final_target<<std::endl;

        // target_yaw = atan2(final_target(1, 0), final_target(0, 0));
        // target_yaw = target_yaw * 180 / M_PI;

        error_distance = sqrt(pow(final_target(0, 0), 2) + pow(final_target(1, 0), 2));
        // error_angle = target_yaw - current_yaw;

        error_angle = atan2(final_target(1, 0), final_target(0, 0));

        error_angle=error_angle*180/M_PI;

        // std::cout<<"error_angle"<<error_angle<<std::endl;

        linear_x = pid_linear.calculate(error_distance, limit_x);
        angular_z = pid_angular.calculate(error_angle, limit_z);

        // // 检查是否是任务点
        // if (points[target_id].tag == "task") {
        //     // 对任务点进行特殊处理
        //     ROS_INFO("到达任务点，执行任务...");
        //     ros::Duration(2.0).sleep(); // 模拟任务执行，等待2秒
        // }

        if(angular_z>0.5) angular_z=0.5;
        if(angular_z<-0.5) angular_z=-0.5;

        if(linear_x>0.1) linear_x=0.1;
        if(linear_x<-0.1) linear_x=-0.1;

        cmd_speed.linear.x = linear_x;
        cmd_speed.angular.z = angular_z;

        if (abs(error_distance) < 0.05) { // 下个目标点判断 


            // 检查是否是任务点
            if (points[target_id].tag == "task") {
                // 
                cmd_speed.linear.x = 0;
                cmd_speed.angular.z = 0;
                pub.publish(cmd_speed);
                ROS_INFO("到达任务点，执行任务...");
                // ros::Duration(2.0).sleep(); // 模拟任务执行，等待2秒
                while(!tasking){
                    ros::spinOnce(); // 检查话题消息
                    ros::Duration(1.0).sleep(); // 等待1秒
                }
                tasking=false;
            }

            target_id++;

            if(target_id==points.size())//终点判断
            {   
                ros::spinOnce(); // 检查里程计消息
                error_angle=0-current_yaw;
                while(abs(error_angle)>5)
                {
                    ROS_INFO("调整角度");
                    ros::spinOnce(); // 检查里程计消息
                    error_angle=0-current_yaw;
                    angular_z = pid_angular.calculate(error_angle, limit_z);
                    // linear_x=0;
                    if(angular_z>0.5) angular_z=0.5;
                    if(angular_z<-0.5) angular_z=-0.5;
                    cmd_speed.linear.x = 0;
                    cmd_speed.angular.z = angular_z;
                    pub.publish(cmd_speed); // 发布控制信息
                }
                target_id++;

            }
            
            if (target_id > points.size()) { // 中止判断
                cmd_speed.linear.x = 0;
                cmd_speed.angular.z = 0;
                pub.publish(cmd_speed);
                ROS_INFO("已到达终点");
                return 1;
            }
            ROS_INFO("下一个目标点是第%d个", target_id);
            std::cout << target[target_id] << std::endl;

            pid_angular.clear_integral();
        }


        if(abs(error_angle)>10 && abs(error_distance)>0.3)  cmd_speed.linear.x = 0;//角度优先
        pub.publish(cmd_speed); // 发布控制信息
        loop_rate.sleep();
    }

    return 0;
}

void getOdom(const nav_msgs::Odometry::ConstPtr& odo_msg) // mid360定位姿态
{
    if(!start_falg) start_falg=1;
    position_x = odo_msg->pose.pose.position.x;
    position_y = odo_msg->pose.pose.position.y;
    current_yaw = odo_msg->pose.pose.orientation.z; // YAW角
}

void get_cargoal(const std_msgs::Bool::ConstPtr& task_msg) // /car_goal
{
    if (task_msg->data) {    
        tasking=true;
    }
    // else{
    //     tasking=0;
    // } 
    
}

// 读取JSON文件并解析目标点数据
std::vector<Point> readJsonFile(const std::string& filename) {
    std::vector<Point> points;

    // 打开文件
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    // 解析JSON
    nlohmann::json jsonData;
    file >> jsonData;
    file.close();

    // 遍历JSON数组并提取目标点
    for (const auto& item : jsonData) {
        Point point;
        point.x = item.at("x").get<double>();
        point.y = item.at("y").get<double>();
        point.tag = item.at("tag").get<std::string>(); // 读取tag字段
        points.push_back(point);
    }

    return points;
}

bool readPIDParams(const std::string& filename, PIDParams& controller1, PIDParams& controller2) {
    // 读取JSON文件
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        return false;
    }

    json jsonData;
    inputFile >> jsonData;

    // 提取PID参数
    controller1.name = jsonData["controllers"][0]["name"];
    controller1.P = jsonData["controllers"][0]["P"];
    controller1.I = jsonData["controllers"][0]["I"];
    controller1.D = jsonData["controllers"][0]["D"];

    controller2.name = jsonData["controllers"][1]["name"];
    controller2.P = jsonData["controllers"][1]["P"];
    controller2.I = jsonData["controllers"][1]["I"];
    controller2.D = jsonData["controllers"][1]["D"];

    return true;
}

void get_stop(const std_msgs::Bool::ConstPtr& stop_msg) // /car_goal获取/stop_signl停障信息
{
    if (stop_msg->data==1)
    {
        stop_flag=true;
    }
    if(stop_msg->data==0 && stop_flag==1)
    {
        stop_flag=false;
        ROS_INFO("恢复停障");
    }
}
