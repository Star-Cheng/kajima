//TEB调试版
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
#include <geometry_msgs/Point.h>
#include <nav_msgs/Path.h>
#include <ros/time.h>

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

int target_id = 0;//全局路径点id
int local_target_id=0;//局部路径点id

double error_local_distance = 0;
double error_local_angle = 0;

double error_global_distance = 0;
double error_global_angle = 0;

double linear_x = 0;
double angular_z = 0;

double target_yaw = 0;

bool tasking=0;//执行任务
bool start_falg=0;//开始标记
bool first_path=0;//第一次接受到path
bool get_path=0;//获取局部路径标记

bool stop_flag=0;//停障信号

// bool once=0;
bool lock=0;//规划锁

bool local_flag=0;//当前全局/局部导航状态

bool planning=0;//是否采用规划
bool planning_sleep=0;//维持信号
bool is_first_stop=0;//是否第一次停止

bool is_turnning=0;//是否在旋转

bool change=0;//是否改变路径

bool fail_planning=0;//是否规划失败

int work_style=0;//工作状态

double limit_x=0.5;//限积分
double limit_z=0.15;
double limit_x_teb=0.20;//限积分

double last_speed_x=0;//上一时刻时速

Eigen::Matrix<double, 3, 1> current_pose; // 当前位置
Eigen::Matrix<double, 3, 3> rotation; // 旋转矩阵

Eigen::Matrix<double, 3, 1> last_pose; // 上一规划时刻位置

std::vector<geometry_msgs::PoseStamped> local_points; // 存储提取的路径点
std::vector<Eigen::Matrix<double, 3, 1>> local_target; // 转换后的全局目标点

geometry_msgs::Twist cmd_speed;

ros::Time last_time;//上一时刻时间
ros::Time current_time;//下一时刻时间

ros::Time start_plan_time;//接收到规划触发信号时间
ros::Time current_plan_time;//当前时间

ros::Time first_stop_time;//第一次遇到障碍物时间
ros::Time current_stop_time;//当前时间

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

void get_stop(const std_msgs::Bool::ConstPtr& stop_msg); // 获取/stop_signl停障信息

void get_plan_fail(const std_msgs::Bool::ConstPtr& fail_msg); // 获取/plan_fail规划失败信息

void get_start_plan(const std_msgs::Bool::ConstPtr& start_plan_msg); // 获取/start_plan开始规划信号

// 读取JSON文件并解析目标点数据
std::vector<Point> readJsonFile(const std::string& filename);
// 读取JSON文件并解析PID参数
bool readPIDParams(const std::string& filename, PIDParams& controller1, PIDParams& controller2);

void toEulerAngle(const Eigen::Quaternionf &q, double &roll, double &pitch, double &yaw);//四元数转欧拉角

void pathCallback(const nav_msgs::Path::ConstPtr& msg);//path回调函数


// PIDParams controller_x_teb;
// PIDParams controller_z_teb;

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");
    ros::init(argc, argv, "control_node");
    ros::NodeHandle nh;
    ros::Subscriber ODOMnewsub = nh.subscribe<nav_msgs::Odometry>("/oula_odom", 10, getOdom); // 获取定位信息
    ros::Publisher pub = nh.advertise<geometry_msgs::Twist>("/smoother_cmd_vel", 10); // 发布控制信息
    ros::Subscriber car_goalsub = nh.subscribe<std_msgs::Bool>("/car_goal", 10, get_cargoal); // 获取/car_goal信息
    ros::Subscriber stopsub = nh.subscribe<std_msgs::Bool>("/stop_signl", 10, get_stop); // 获取/stop_signl停障信息

    ros::Publisher teb_target_pub = nh.advertise<geometry_msgs::Point>("/teb_target_point", 10); // 发布当前目标点
    ros::Subscriber pathsub = nh.subscribe<nav_msgs::Path>("/map_update_with_teb/teb_planner/local_plan", 10, pathCallback); // 订阅path消息

    ros::Subscriber plan_fail_sub = nh.subscribe<std_msgs::Bool>("/plan_fail", 10, get_plan_fail); // 获取/plan_fail规划失败信息
    ros::Subscriber start_plan_sub = nh.subscribe<std_msgs::Bool>("/start_plan", 10, get_start_plan); // 获取/start_plan开始规划信号

    ros::Rate loop_rate(20);
    PIDParams controller_x;
    PIDParams controller_z;

    PIDParams controller_x_teb;
    PIDParams controller_z_teb;

    if (readPIDParams("/home/agile/ws_robot/control_ws/src/bunker_control/src/pid_params_teb.json", controller_x_teb, controller_z_teb)) {
        // 输出读取到的PID参数
        std::cout << controller_x_teb.name << std::endl;
        std::cout << "P: " << controller_x_teb.P << std::endl;
        std::cout << "I: " << controller_x_teb.I << std::endl;
        std::cout << "D: " << controller_x_teb.D << std::endl;

        std::cout << controller_z_teb.name << std::endl;
        std::cout << "P: " << controller_z_teb.P << std::endl;
        std::cout << "I: " << controller_z_teb.I << std::endl;
        std::cout << "D: " << controller_z_teb.D << std::endl;
    } else {
        std::cerr << "Failed to read PID parameters from JSON file" << std::endl;
        return 1;
    }

    if (readPIDParams("/home/agile/ws_robot/control_ws/src/bunker_control/src/pid_params.json", controller_x, controller_z)) {
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
        std::string filename = "/home/agile/ws_robot/map/map_now/target.json";
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

    // Eigen::Matrix<double, 3, 3> rotation; // 旋转矩阵

    rotation << cos(current_yaw), sin(current_yaw), 0,
        (-sin(current_yaw)), cos(current_yaw), 0,
        0, 0, 1; // 旋转矩阵

    // Eigen::Matrix<double, 3, 1> current_pose; // 当前位置
    current_pose << position_x, position_y, 0;

    Eigen::Matrix<double, 3, 1> final_target; // 当前目的坐标点
    Eigen::Matrix<double, 3, 1> global_target; // 当前全局目的坐标点
    // Eigen::Matrix<double, 3, 1> local_target; // 当前目的坐标点

    geometry_msgs::Point target_msg; // 发布局部规划目标点
    target_msg.z = 0;

    PID pid_linear(controller_x.P, controller_x.I, controller_x.D); // PID参数需要根据实际情况调整
    PID pid_angular(controller_z.P, controller_z.I, controller_z.D);

    PID pid_linear_teb(controller_x_teb.P, controller_x_teb.I, controller_x_teb.D); // PID参数需要根据实际情况调整
    PID pid_angular_teb(controller_z_teb.P, controller_z_teb.I, controller_z_teb.D);

    ros::Duration diff_plan;
    ros::Duration diff_first_stop;
    double linear_limit=0;//计算线性限速

    ROS_INFO("初始化完成");
    while(!start_falg)//等待里程计消息
    {
        ros::spinOnce(); // 检查里程计消息       
        ROS_INFO("等待里程计消息");
        ros::Duration(1.0).sleep(); // 等待1秒
    }

        target_msg.x = target[target_id](0, 0);
        target_msg.y = target[target_id](1, 0);
        teb_target_pub.publish(target_msg); // 发布目标点

    while(!first_path)//等待路径消息
    {
        target_msg.x = target[target_id](0, 0);
        target_msg.y = target[target_id](1, 0);
        teb_target_pub.publish(target_msg); // 发布目标点
        ros::Duration(1.0).sleep(); // 等待1秒
        ros::spinOnce(); // 检查里程计消息       
        ROS_INFO("等待局部路径");
        // ros::Duration(1.0).sleep(); // 等待1秒
    }

    // std::cout<<"1111"<<std::endl;


    // while(!get_path)//等待路径消息
    // {
    //     target_msg.x = target[target_id](0, 0);
    //     target_msg.y = target[target_id](1, 0);
    //     teb_target_pub.publish(target_msg); // 发布目标点
    //     ros::Duration(1.0).sleep(); // 等待1秒
    //     ros::spinOnce(); // 检查里程计消息       
    //     ROS_INFO("等待局部路径");
    //     // ros::Duration(1.0).sleep(); // 等待1秒
    // }

    if (points[target_id].tag == "normal")
    {
        work_style=0;
    }
    else if (points[target_id].tag == "narrow")//狭窄
    {
        work_style=1;
    }
    else if (points[target_id].tag == "door")//过门
    {
        work_style=2;
    }

    while (ros::ok()) {
        ros::spinOnce(); // 检查里程计消息

            // std::cout<<"1111"<<std::endl;

        while (stop_flag)// 停障
        {
            ROS_INFO("停障");
            ros::spinOnce(); // 检查里程计消息
            cmd_speed.linear.x = 0;       
            cmd_speed.angular.z = 0;
            pub.publish(cmd_speed); // 发布控制信息
            last_speed_x=0;
            ros::Duration(1.0).sleep(); // 等待1秒
            pid_angular.clear_integral();
            pid_angular_teb.clear_integral();
            pid_linear.clear_integral();
            pid_linear_teb.clear_integral();
        }

        if(target_id>0)
        {
            if(sqrt(pow((position_x-target[target_id-1](0,0)), 2) + pow((position_y-target[target_id-1](1,0)), 2)) < 0.5)
            {
                is_turnning=true;
            }
            else
            {
                is_turnning=false;
            }
        }

        // if (points[target_id].tag == "normal")
        // {
        //     work_style=0;
        // }
        // else if (points[target_id].tag == "narrow")//狭窄
        // {
        //     work_style=1;
        // }
        // else if (points[target_id].tag == "door")//过门
        // {
        //     work_style=2;
        // }
        
        

        if(fail_planning==true && planning==true && is_turnning==false && work_style==0)//规划失败
        {
            cmd_speed.linear.x = 0;       
            cmd_speed.angular.z = 0;
            pub.publish(cmd_speed); // 发布控制信息

            while (fail_planning==true)
            {
                ros::spinOnce(); // 检查里程计消息   
                cmd_speed.linear.x = 0;       
                cmd_speed.angular.z = 0;
                pub.publish(cmd_speed); // 发布控制信息
                ros::Duration(1.0).sleep(); // 等待1秒
            }

            get_path=0;
            
        }

        if(is_first_stop==false && planning==true && is_turnning==false && work_style==0)
        {
            first_stop_time=ros::Time::now();//记录当前时刻
            is_first_stop=true;

            std::cout<<"首次发现障碍物"<<std::endl;
            cmd_speed.linear.x = 0;       
            cmd_speed.angular.z = 0;
            pub.publish(cmd_speed); // 发布控制信息
            ros::Duration(1.0).sleep(); // 等待1秒
            get_path=0;
            last_speed_x=0;
            ros::Duration(2.0).sleep(); // 等待1秒
            ros::spinOnce(); // 检查里程计消息                 
            while(!get_path)//等待路径消息
            {
                target_msg.x = target[target_id](0, 0);
                target_msg.y = target[target_id](1, 0);
                teb_target_pub.publish(target_msg); // 发布目标点
                ros::Duration(1.0).sleep(); // 等待1秒
                ros::spinOnce(); // 检查里程计消息       
                ROS_INFO("等待局部路径");
                cmd_speed.linear.x = 0;       
                cmd_speed.angular.z = 0;
                pub.publish(cmd_speed); // 发布控制信息
                // ros::Duration(1.0).sleep(); // 等待1秒
            }
                
            pid_angular.clear_integral();
            pid_angular_teb.clear_integral();
            pid_linear.clear_integral();
            pid_linear_teb.clear_integral();
            // first_stop_time=ros::Time::now();//记录当前时刻
        }
        else if(is_first_stop==true)
        {
            current_stop_time=ros::Time::now();
            diff_first_stop=current_stop_time-first_stop_time;
            if(diff_first_stop.toSec() >= 15.0)
            {
                is_first_stop==false;
            }
        }


        if(planning==true && is_turnning==false && work_style==0)
        {
            current_plan_time=ros::Time::now();//记录当前时刻            
            diff_plan = current_plan_time - start_plan_time;
            // std::cout<<"1"<<std::endl;
            // std::cout<<diff_plan.toSec()<<std::endl;

            while(!get_path)//等待路径消息
            {
                target_msg.x = target[target_id](0, 0);
                target_msg.y = target[target_id](1, 0);
                teb_target_pub.publish(target_msg); // 发布目标点
                // ros::Duration(1.0).sleep(); // 等待1秒
                ros::spinOnce(); // 检查里程计消息       
                ROS_INFO("等待局部路径");
                cmd_speed.linear.x = 0;       
                cmd_speed.angular.z = 0;
                pub.publish(cmd_speed); // 发布控制信息
                // ros::Duration(1.0).sleep(); // 等待1秒
                last_speed_x=0;
            }

            if(planning_sleep==false)
            {
                // std::cout<<"首次发现障碍物"<<std::endl;
                // cmd_speed.linear.x = 0;       
                // cmd_speed.angular.z = 0;
                // pub.publish(cmd_speed); // 发布控制信息
                // ros::Duration(1.0).sleep(); // 等待1秒
                get_path=0;
                // ros::Duration(2.0).sleep(); // 等待1秒
                ros::spinOnce(); // 检查里程计消息                 
                // last_speed_x=0;
                while(!get_path)//等待路径消息
                {
                    target_msg.x = target[target_id](0, 0);
                    target_msg.y = target[target_id](1, 0);
                    teb_target_pub.publish(target_msg); // 发布目标点
                    // ros::Duration(1.0).sleep(); // 等待1秒
                    ros::spinOnce(); // 检查里程计消息       
                    ROS_INFO("等待局部路径");
                    cmd_speed.linear.x = 0;       
                    cmd_speed.angular.z = 0;
                    pub.publish(cmd_speed); // 发布控制信息
                    // ros::Duration(1.0).sleep(); // 等待1秒
                }
                // ros::Duration(2.0).sleep(); // 等待1秒
                planning_sleep=true;
                last_speed_x=0;
                pid_angular.clear_integral();
                pid_angular_teb.clear_integral();
                pid_linear.clear_integral();
                pid_linear_teb.clear_integral();
            }
            if(diff_plan.toSec() >= 15.0)
            {
                planning=false;
                get_path=false;
                planning_sleep=false;
                std::cout<<"退出局部路径"<<std::endl;
                pid_linear_teb.clear_integral();
                pid_angular_teb.clear_integral();
            }        
        }

        if(change==1)//改变路径
        {
            change=0;
            pid_angular.clear_integral();//清除角度积分
            pid_angular_teb.clear_integral();
        }

        

        last_speed_x=linear_x;//记录上一时刻速度

        

        // doubule current_angle=current_yaw
        rotation << cos(current_yaw/ 180 * M_PI), sin(current_yaw/ 180 * M_PI), 0,
            (-sin(current_yaw/ 180 * M_PI)), cos(current_yaw/ 180 * M_PI), 0,
            0, 0, 1; // 更新旋转矩阵


        current_pose << position_x, position_y, 0; // 更新位置



        // final_target = rotation*(target[target_id] - current_pose);//计算当前目标点
        global_target = rotation*(target[target_id] - current_pose);//计算全局目标点
        // // 发布TEB当前目标点
        // target_msg.x = target[target_id](0, 0);
        // target_msg.y = target[target_id](1, 0);
        // // std::cout<<"global target"<<target[target_id]<<std::endl;
        // teb_target_pub.publish(target_msg); // 发布目标点

        // final_target(0,0)=local_points[local_target_id].pose.position.x;
        // final_target(1,0)=local_points[local_target_id].pose.position.y;

        // final_target=rotation*(local_target[local_target_id]-current_pose);

        // std::cout<<"当前目标点"<<final_target<<std::endl;

        if(planning && get_path && work_style==0)
        {
            final_target=rotation*(local_target[local_target_id]-current_pose);

            error_local_distance = sqrt(pow(final_target(0, 0), 2) + pow(final_target(1, 0), 2));

            error_local_angle = atan2(final_target(1, 0), final_target(0, 0));

            if(final_target(1, 0)<0 && final_target(0, 0)<0)
            {
                error_local_angle=error_local_angle-M_PI;
            }
            else if(final_target(1, 0)>0 && final_target(0, 0)<0)
            {
                error_local_angle=error_local_angle+M_PI;
            }

            error_local_angle=error_local_angle*180/M_PI;
        }

        // error_local_distance = sqrt(pow(final_target(0, 0), 2) + pow(final_target(1, 0), 2));

        // error_local_angle = atan2(final_target(1, 0), final_target(0, 0));

        // error_local_angle=error_local_angle*180/M_PI;

        error_global_distance = sqrt(pow(global_target(0, 0), 2) + pow(global_target(1, 0), 2));

        error_global_angle = atan2(global_target(1, 0), global_target(0, 0));

        if(global_target(1, 0)<0 && global_target(0, 0)<0)
        {
            error_global_angle=error_global_angle-M_PI;
        }
        else if(global_target(1, 0)>0 && global_target(0, 0)<0)
        {
            error_global_angle=error_global_angle+M_PI;
        }

        error_global_angle=error_global_angle*180/M_PI;



        // std::cout<<"error_angle "<<error_angle<<std::endl;
        // std::cout<<"全局误差"<<sqrt(pow(global_target(0, 0), 2) + pow(global_target(1, 0), 2))<<std::endl;

        if(error_global_distance < 1.0 )//离全局目标点还差半米不再采用局部路径点 或者已经没有局部路径点
        {

            if(target_id<points.size()-1)//提前发布下一个目标点
            {
                target_msg.x = target[target_id+1](0, 0);
                target_msg.y = target[target_id+1](1, 0);
                // std::cout<<"global target"<<target[target_id]<<std::endl;
                teb_target_pub.publish(target_msg); // 发布目标点
                // linear_x = pid_linear.calculate(error_global_distance, limit_x);
                // angular_z = pid_angular.calculate(error_global_angle, limit_z);
                // std::cout<<"接近点导航中"<<std::endl;
            }

            linear_x = pid_linear.calculate(error_global_distance, limit_x);
            angular_z = pid_angular.calculate(error_global_angle, limit_z);
            std::cout<<"接近点导航中"<<std::endl;
            // std::cout<<"x"<<linear_x<<std::endl;
            local_flag=0;
            // std::cout<<"error"<<error_global_distance<<std::endl;
        }
        else if(target_id>0 && sqrt(pow((position_x-target[target_id-1](0,0)), 2) + pow((position_y-target[target_id-1](1,0)), 2)) < 0.5)
        {
            linear_x = pid_linear.calculate(error_global_distance, limit_x);
            angular_z = pid_angular.calculate(error_global_angle, limit_z);
            std::cout<<"离去导航中"<<std::endl;
            local_flag=0;
            // 发布TEB当前目标点
            target_msg.x = target[target_id](0, 0);
            target_msg.y = target[target_id](1, 0);
            // std::cout<<"global target"<<target[target_id]<<std::endl;
            teb_target_pub.publish(target_msg); // 发布目标点
            get_path=false;
        }
        // else if(get_path==false)
        // {
        //     target_msg.x = target[target_id](0, 0);
        //     target_msg.y = target[target_id](1, 0);
        //     // std::cout<<"global target"<<target[target_id]<<std::endl;
        //     teb_target_pub.publish(target_msg); // 发布目标点
        //     linear_x = pid_linear.calculate(error_global_distance, limit_x);
        //     angular_z = pid_angular.calculate(error_global_angle, limit_z);
        //     std::cout<<"全局路径导航中"<<std::endl;
        //     // std::cout<<target_id<<std::endl;
        //     local_flag=0;
        // }
        else if(planning==true && get_path==true &&work_style==0 )//局部导航
        {
            // 发布TEB当前目标点
            target_msg.x = target[target_id](0, 0);
            target_msg.y = target[target_id](1, 0);
            // std::cout<<"global target"<<target[target_id]<<std::endl;
            teb_target_pub.publish(target_msg); // 发布目标点
            // std::cout<<"局部路径导航中"<<std::endl;
            linear_x = pid_linear_teb.calculate(error_local_distance, limit_x_teb);
            angular_z = pid_angular_teb.calculate(error_local_angle, limit_z);
            local_flag=1;
            // std::cout<<"error_local_distance "<<error_local_distance<<std::endl;
            // std::cout<<"error_local_angle "<<error_local_angle<<std::endl;
            // std::cout<<local_target[local_target_id]<<std::endl;
        }
        else//全局导航
        {
            target_msg.x = target[target_id](0, 0);
            target_msg.y = target[target_id](1, 0);
            // std::cout<<"global target"<<target[target_id]<<std::endl;
            teb_target_pub.publish(target_msg); // 发布目标点
            linear_x = pid_linear.calculate(error_global_distance, limit_x);
            angular_z = pid_angular.calculate(error_global_angle, limit_z);
            std::cout<<"全局路径导航中"<<std::endl;
            // std::cout<<target_id<<std::endl;
            local_flag=0;
        }

        // std::cout<<"error_local_distance "<<error_local_distance<<std::endl;
        // std::cout<<"error_local_angle "<<error_local_angle<<std::endl;

        // std::cout<<"error_global_distance "<<error_global_distance<<std::endl;
        // std::cout<<"error_global_angle "<<error_global_angle<<std::endl;

        // std::cout<<"linear_x"<<linear_x<<std::endl;
        // std::cout<<"linear_x"<<angular_z<<std::endl;



        if(angular_z>0.3) angular_z=0.3;
        if(angular_z<-0.3) angular_z=-0.3;

        // if(linear_x>0.3) linear_x=0.3;
        // if(linear_x<-0.3) linear_x=-0.3;

        if(planning==true && work_style==0)
        {
            if(linear_x>0.3) linear_x=0.3;
            if(linear_x<-0.3) linear_x=-0.3;
        }
        else if(work_style==1)//狭窄
        {
            if(linear_x>0.3) linear_x=0.3;
            if(linear_x<-0.3) linear_x=-0.3;
        }
        else if(work_style==2)//过门
        {
            if(linear_x>0.3) linear_x=0.3;
            if(linear_x<-0.3) linear_x=-0.3;
        }
        else
        {
            if(linear_x>0.3) linear_x=0.3;
            if(linear_x<-0.3) linear_x=-0.3;
        }


        if(abs(error_global_distance)<1.0 && abs(error_global_distance)>=0.2 )
        {
            linear_limit=(error_global_distance-0.2)*(0.3-0.1)/(1-0.2)+0.1;//线性限速0.3-0.1 1-0.2m
            if(linear_x>linear_limit) linear_x=linear_limit;
            if(linear_x<(-1)*linear_limit) linear_x=(-1)*linear_limit;
        }
        else if(abs(error_global_distance)<0.2)
        {
            if(linear_x>0.1) linear_x=0.1;
            if(linear_x<-0.1) linear_x=-0.1;
        }

        if( linear_x-last_speed_x > (0.3/20) ) linear_x=last_speed_x+(0.3/20);
        // if( linear_x-last_speed_x < -(0.3/20) ) linear_x=last_speed_x-(0.3/20);

        cmd_speed.linear.x = linear_x;
        cmd_speed.angular.z = angular_z;

        if(abs(error_global_angle)>10  && local_flag==0 )  
        {
            cmd_speed.linear.x = 0;//全局角度优先
            std::cout<<"角度优先"<<std::endl;
        }

        if(abs(error_global_angle)>5  && work_style==2)//过门
        {
            cmd_speed.linear.x = 0;//全局角度优先
            std::cout<<"过门角度优先"<<std::endl;
        }

        if(abs((error_local_angle)>5) && local_flag==1 ) cmd_speed.linear.x = 0;//全局角度优先

        

        if ((abs(error_local_distance) < 0.05 && planning && get_path && work_style==0) || error_global_distance <0.05) { // 下个目标点判断 


            // // 检查是否是任务点
            // if (points[target_id].tag == "task") {
            //     // 
            //     cmd_speed.linear.x = 0;
            //     cmd_speed.angular.z = 0;
            //     pub.publish(cmd_speed);
            //     ROS_INFO("到达任务点，执行任务...");
            //     // ros::Duration(2.0).sleep(); // 模拟任务执行，等待2秒
            //     while(!tasking){
            //         ros::spinOnce(); // 检查话题消息
            //         ros::Duration(1.0).sleep(); // 等待1秒
            //     }
            //     tasking=false;
            // }

            if(planning && get_path) local_target_id++;

            pid_angular_teb.clear_integral();

            if(local_target_id>=local_points.size())//到达全局路径最后一个点
            {
                get_path=0;
            }


            if(error_global_distance < 0.05)//如果到达全局路径点
            {
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
                if(target_id<points.size())
                {
                    // teb_target = rotation*(target[target_id] - current_pose);//计算teb目标点
                    // 发布TEB当前目标点
                    target_msg.x = target[target_id](0, 0);
                    target_msg.y = target[target_id](1, 0);
                    teb_target_pub.publish(target_msg); // 发布目标点 

                    if (points[target_id].tag == "normal")
                    {
                        work_style=0;
                        ROS_INFO("NORMAL");
                    }
                    else if (points[target_id].tag == "narrow")//狭窄
                    {
                        work_style=1;
                        ROS_INFO("NARROW");
                    }
                    else if (points[target_id].tag == "door")//过门
                    {
                        work_style=2;
                        ROS_INFO("DOOR");
                    }
                }

                pid_angular.clear_integral();
            }


            if(target_id==points.size())//终点判断
            {   
                ros::spinOnce(); // 检查里程计消息
                error_global_angle=0-current_yaw;
                while(abs(error_global_angle)>5)
                {
                    ROS_INFO("调整角度");
                    ros::spinOnce(); // 检查里程计消息
                    error_global_angle=0-current_yaw;
                    angular_z = pid_angular.calculate(error_global_angle, limit_z);
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

            // if(local_target_id==local_points.size())//局部路径点走完
            // {

            // }

            ROS_INFO("下一个全局目标点是第%d个", target_id);
            std::cout << target[target_id] << std::endl;


            if(planning==true && get_path==true &&work_style==0)
            {
                ROS_INFO("下一个局部目标点是第%d个", local_target_id);
                std::cout << final_target(0,0) <<" "<<final_target(1,0)<< std::endl;                
            }


            // pid_angular.clear_integral();
        }


        // if(abs(error_angle)>10 && abs(error_distance)>0.3)  cmd_speed.linear.x = 0;//角度优先
        pub.publish(cmd_speed); // 发布控制信息
        // std::cout<<"x "<<cmd_speed.linear.x<<std::endl;
        // std::cout<<"z "<<cmd_speed.angular.z<<std::endl;
        loop_rate.sleep();
    }

    return 0;
}

void getOdom(const nav_msgs::Odometry::ConstPtr& odo_msg) // mid360定位姿态
{
    if(!start_falg) start_falg=1;
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

void get_stop(const std_msgs::Bool::ConstPtr& stop_msg) // 获取/stop_signl停障信息
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

void pathCallback(const nav_msgs::Path::ConstPtr& msg)//path回调函数
{
        // std::cout<<"333"<<std::endl;
    if(!first_path) first_path=true;
    if(planning==false) 
    {   
        // std::cout<<"222"<<std::endl;
        return;
    }

    // std::cout<<"ss"<<std::endl;
    if(!get_path) 
    {
        get_path=1;//已获取path
        last_pose(0,0)=position_x;
        last_pose(1,0)=position_y;//
        last_time=ros::Time::now();
        lock=true;  

    }
    else
    {
        current_time=ros::Time::now();
        ros::Duration diff = current_time - last_time;

        if(!lock)
        {
            last_pose(0,0)=position_x;
            last_pose(1,0)=position_y;//
            last_time=ros::Time::now();
            lock=true;        
        }
        else if( lock && (sqrt(pow(position_x-last_pose(0,0), 2) + pow(position_y-last_pose(1,0), 2))>=2.0 || diff.toSec() >= 10.0 ))
        {
            std::cout<<"切换轨迹"<<std::endl;
            lock=false;
            return;
        }
        else
        {
            return;
        }
    }
    // if(!once) 
    // {
    //     once=true;
    // }
    // else
    // {
    //     return;
    // }

    // current_time=ros::Time::now();
    // ros::Duration diff = current_time - last_time;

    // if(!lock)
    // {
    //     last_pose(0,0)=position_x;
    //     last_pose(1,0)=position_y;//
    //     last_time=ros::Time::now();
    //     lock=true;        
    // }
    // else if( lock && (sqrt(pow(position_x-last_pose(0,0), 2) + pow(position_y-last_pose(1,0), 2))>=2.5 || diff.toSec() >= 10.0 ))
    // {
    //     std::cout<<"切换轨迹"<<std::endl;
    //     lock=false;
    //     return;
    // }
    // else
    // {
    //     return;
    // }

    if(change==0) change==1;

    // 清空原来的路径点
    local_points.clear();
    local_target_id=0;//局部路径id清零
    local_target.clear();//转换后的局部目标

    // pid_angular.clear_integral();//清除角度积分
    // pid_angular_teb.clear_integral();
    
    // 从消息中获取路径点
    const auto& poses = msg->poses;
    int size = poses.size();
    
    // 每隔5个点取一个，并确保取到最后一个点
    for (int i = 1; i < size; i += 3) {
        local_points.push_back(poses[i]);
    }
    
    // 如果最后一个点没有被取到，手动添加
    if (size > 0 && (size - 1) % 3 != 0) {
        local_points.push_back(poses[size - 1]);
    }

    rotation << cos(current_yaw/ 180 * M_PI), sin(current_yaw/ 180 * M_PI), 0,
        (-sin(current_yaw/ 180 * M_PI)), cos(current_yaw/ 180 * M_PI), 0,
        0, 0, 1; // 更新旋转矩阵


    current_pose << position_x, position_y, 0; // 更新位置

    Eigen::Matrix<double, 3, 1> tansfer;
    tansfer(2,0)=0;

    for(int j=0; j<local_points.size(); ++j)
    {
        tansfer(0,0)=local_points[j].pose.position.x;
        tansfer(1,0)=local_points[j].pose.position.y;

        tansfer=current_pose+rotation.inverse()*tansfer;

        local_target.push_back(tansfer);
    }




    // geometry_msgs::PoseStamped last_point;//全局目标点

    // local_points.push_back(last_point);

    // 打印取到的点
    ROS_INFO("local_points size: %zu", local_points.size());
    // for (const auto& point : local_points) {
    //     ROS_INFO("Position: [%.2f, %.2f, %.2f]", point.pose.position.x, point.pose.position.y, point.pose.position.z);
    // }
}

void get_plan_fail(const std_msgs::Bool::ConstPtr& fail_msg) // 获取/plan_fail规划失败信息
{
    if (fail_msg->data==1)
    {
        ROS_INFO("规划失败");
        fail_planning=true;
    }
    else if(fail_msg->data==0)
    {
        fail_planning=false;  
    }
}

void get_start_plan(const std_msgs::Bool::ConstPtr& start_plan_msg) // 获取/start_plan开始规划信号
{
    if (start_plan_msg->data==1)
    {
        // ROS_INFO("开始采用规划");
        start_plan_time=ros::Time::now();
        planning=true;
    }
    // else if(start_plan_msg->data==0)
    // {
    //     planning=false;
    // }
}