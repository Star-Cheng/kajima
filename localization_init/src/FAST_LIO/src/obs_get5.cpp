#include <omp.h>
#include <mutex>
#include <math.h>
#include <thread>
#include <fstream>
#include <csignal>
#include <unistd.h>
#include <Python.h>
#include <so3_math.h>
#include <ros/ros.h>
#include <Eigen/Core>
#include "IMU_Processing.hpp"
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>
#include <visualization_msgs/Marker.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <geometry_msgs/Vector3.h>
#include <livox_ros_driver2/CustomMsg.h>
#include "preprocess.h"
#include <ikd-Tree/ikd_Tree.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_broadcaster.h>
#include <nlohmann/json.hpp>

#include "std_msgs/Bool.h"
#include <pcl/filters/extract_indices.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/segmentation/extract_clusters.h>




string root_dir = ROOT_DIR;
//////

double x_g , x_g_r , y_g_r , z_g_r;
double y_g ;
double z_g ;
int grav_cnt = 0;
Eigen::Quaternionf rotation_quaternion_grav;

float rotation_angle_x;
float rotation_angle_y;
Eigen::Quaternionf get_gravity_align_rotation(Eigen::Vector3f grav_world) 
{
    if (grav_world.z() < 0) {
        grav_world = -grav_world;
    }
    //grav_world.normalize();
    float x_grav = grav_world(0);
    Eigen::Vector3f x_gravity(x_grav, 0.0f, 0.0f);
    float y_grav = grav_world(1);
    Eigen::Vector3f y_gravity(0.0f, y_grav, 0.0f);

    Eigen::Vector3f grav_rm_x = grav_world - x_gravity;
    Eigen::Vector3f grav_rm_y = grav_world - y_gravity;
    Eigen::Vector3f z_axis(0, 0, 1);
    Eigen::Vector3f x_axis(1, 0, 0);
    Eigen::Vector3f y_axis(0, 1, 0);


    float cos_theta_x = grav_rm_x.dot(z_axis) / (grav_rm_x.norm() * z_axis.norm());
    float sin_theta_x = std::sqrt(1.0f - cos_theta_x*cos_theta_x);
    rotation_angle_x = std::atan2(sin_theta_x, cos_theta_x);
    //  std::cout << "Rotation angle:\n"
    //           << rotation_angle_x << std::endl;

    float cos_theta_y = grav_rm_y.dot(z_axis) / (grav_rm_y.norm() * z_axis.norm());
    float sin_theta_y = std::sqrt(1.0f - cos_theta_y*cos_theta_y);
    rotation_angle_y = std::atan2(sin_theta_y, cos_theta_y);
    //  std::cout << "Rotation angle:\n"
    //           << rotation_angle_y << std::endl;

    Eigen::Quaternionf rotation_quaternion_x , rotation_quaternion_y,rotation_quaternion;
    rotation_quaternion_x = Eigen::AngleAxisf(rotation_angle_x, x_axis);
    rotation_quaternion_y = Eigen::AngleAxisf(rotation_angle_y, y_axis);
    rotation_quaternion = rotation_quaternion_y*rotation_quaternion_x;
    return rotation_quaternion;

}


bool acc_grav()
{
    if (grav_cnt == 11)
    {
        // std::cout << "x_g_r: " << x_g_r << " y_g_r: " << y_g_r << "z_g_r:" <<z_g_r<<std::endl;
        Eigen::Vector3f grav_world(x_g_r, y_g_r, z_g_r);
        rotation_quaternion_grav = get_gravity_align_rotation(grav_world);
        return true;
    }else
    {
        return false;
    }
}

void pointstran_grav(PointType const * const pi, PointType * const po)
{
   
    Eigen::Vector3f p_body_lidar(pi->x, pi->y, pi->z);
    Eigen::Vector3f p_lidar_grav(rotation_quaternion_grav*p_body_lidar );

    po->x = p_lidar_grav(0);
    po->y = p_lidar_grav(1);
    po->z = p_lidar_grav(2);
    po->intensity = pi->intensity;

}


/////计算到目标点


geometry_msgs::Point cur_target;
nav_msgs::Odometry cur_odom;
void pointCallback(const geometry_msgs::Point::ConstPtr& msg)
{
    cur_target = *msg;

}

void odom_cbk(const nav_msgs::Odometry::ConstPtr &msg)
{
    cur_odom = *msg;
}

/////////

////角度计
void pointstran_grav1(PointType const * const pi, pcl::PointXYZI * const po)
{
   
    Eigen::Vector3f p_body_lidar(pi->x, pi->y, pi->z);
    Eigen::Vector3f p_lidar_grav(rotation_quaternion_grav*p_body_lidar );

    po->x = p_lidar_grav(0);
    po->y = p_lidar_grav(1);
    po->z = p_lidar_grav(2);
    po->intensity = pi->intensity;

}

double calculateAngleWithXAxis(PointType const * const pi) {
    // 提取点的x, y, z坐标
    double x = pi->x;
    double y = pi->y;
    double z = pi->z;

    // 使用atan2计算与x轴正方向向量的角度
    double angle_rad = atan2(y, x); // atan2返回的是弧度值，范围是[-π, π]
    double angle_deg = angle_rad/3.1415*180; // 转换为度

    // 根据atan2的返回值调整角度范围，确保它是0到360度之间
    // if (angle_deg < 0) {
    //     angle_deg += 360.0;
    // }

    return angle_deg;
}

double calculateMagnitude(PointType const * const pi) {
    if (pi == nullptr) {
        throw std::invalid_argument("Point pointer is null");
    }
    
    // 计算模长：sqrt(x^2 + y^2 + z^2)
    double magnitude = std::sqrt(pi->x * pi->x + pi->y * pi->y + pi->z * pi->z);
    return magnitude;
}

////检测是否在矩形内/////

// 定义一个结构体来表示二维点
struct RectanglePoint {
    double x;
    double y;
};

// 函数用于检测点是否在矩形内
// 矩形由两个对角顶点topLeft和bottomRight定义
template<typename PointType>
bool isPointInRectangle(const PointType& point, 
                         const RectanglePoint& topLeft, 
                         const RectanglePoint& bottomRight) 
{
    // 检查点的x坐标是否在矩形的左边和右边之间
    bool withinX = point.x >= topLeft.x && point.x <= bottomRight.x;
    // 检查点的y坐标是否在矩形的顶部和底部之间
    bool withinY = point.y >= bottomRight.y && point.y <= topLeft.y;

    // 如果点的x和y坐标都在矩形的边界内，则点在矩形内
    return withinX && withinY;
}


shared_ptr<Preprocess> p_pre(new Preprocess());
deque<pcl::PointCloud<pcl::PointXYZI>::Ptr>    obstacle_buffer;
void livox_pcl_cbk(const livox_ros_driver2::CustomMsg::ConstPtr &msg) 
{
    if (acc_grav())
    {

        PointCloudXYZI::Ptr  ptr(new PointCloudXYZI());
        p_pre->process(msg, ptr);
        int size_init_cloud = ptr->size();
        int icout = 0;
        int icout1 = 0;
        PointCloudXYZI::Ptr laserCloud_tran_grav1(new PointCloudXYZI(size_init_cloud, 1));
        pcl::PointCloud<pcl::PointXYZI>::Ptr obstacle_cloud(new pcl::PointCloud<pcl::PointXYZI>(size_init_cloud, 1));        
        
        double xx = cur_odom.pose.pose.position.x-cur_target.x;
        double yy = cur_odom.pose.pose.position.y-cur_target.y;
        double d_target = std::sqrt(xx*xx + yy*yy);
        ////
        RectanglePoint car_topLeft = {-1, 0.3};
        RectanglePoint car_bottomRight = {0.1, -0.3};

        ////
        if (d_target > 2)
        {
            d_target = 2;
        }
        
        RectanglePoint topLeft = {-0.2, 0.4};
        RectanglePoint bottomRight = {0.1, -0.4};
        RectanglePoint topLeft1 = {0.1, 0.3};
        RectanglePoint bottomRight1 = {d_target, -0.3};
        
        for (size_t i = 0; i < size_init_cloud; i++)
        {
            double angle = calculateAngleWithXAxis(&ptr->points[i]);
            double distance = calculateMagnitude(&ptr->points[i]);
            

            pointstran_grav(&ptr->points[i], \
                            &laserCloud_tran_grav1->points[icout]);

            if (isPointInRectangle(laserCloud_tran_grav1->points[icout] ,car_topLeft, car_bottomRight))
            {
                continue;
            }
            
            if ((isPointInRectangle(laserCloud_tran_grav1->points[icout] , topLeft, bottomRight)|| isPointInRectangle(laserCloud_tran_grav1->points[icout] , topLeft1, bottomRight1)) && laserCloud_tran_grav1->points[icout].z>(-0.1) && laserCloud_tran_grav1->points[icout].z<3)
            {
                pointstran_grav1(&ptr->points[i], \
                            &obstacle_cloud->points[icout1]);
                icout1++;
            }
            icout++;
        }
        obstacle_cloud->points.resize(icout1);
        obstacle_buffer.push_back(obstacle_cloud);
        
        if (obstacle_buffer.size()>2)
        {
            obstacle_buffer.pop_front();
        }
        


    }
    
    
}


void imu_cbk(const sensor_msgs::Imu::ConstPtr &msg_in) 
{
    if (grav_cnt <10)
    {
        x_g += msg_in->linear_acceleration.x;
        y_g += msg_in->linear_acceleration.y;
        z_g += msg_in->linear_acceleration.z;
        grav_cnt++;
    }
    if (grav_cnt ==10)
    {
        x_g_r = x_g/(grav_cnt);
        y_g_r = y_g/(grav_cnt);
        z_g_r = z_g/(grav_cnt);
        grav_cnt++;
    }
   
    if (acc_grav())
    {

    }

}


////////
bool stop_flag = false;
void publish_stop(const ros::Publisher & bool_pub)
{
    std_msgs::Bool msg;
    msg.data = stop_flag; // 或者根据需要设置为false
    bool_pub.publish(msg);

}



Eigen::Vector3f calculateXYZCentroid(const pcl::PointCloud<pcl::PointXYZI>& cloud) {
    Eigen::Vector3f centroid;
    centroid.setZero(); // 初始化xyz平面上的质心为零向量

    // 遍历点云，累加所有点的x和y坐标
    for (const pcl::PointXYZI& point : cloud) {
        centroid[0] += point.x; // 累加x坐标
        centroid[1] += point.y; // 累加y坐标
        centroid[2] += point.z; // 累加z坐标

    }

    // 将累加的坐标除以点的数量，得到xy平面上的中心坐标
    float size = static_cast<float>(cloud.size());
    centroid[0] = centroid[0]/size;
    centroid[1] = centroid[1]/size;
    centroid[2] = centroid[2]/size;

    return centroid;
}



/////点云分割///
double obstacle_distance = 0.6;
pcl::PointCloud<pcl::PointXYZI>::Ptr seg_A_C (new pcl::PointCloud<pcl::PointXYZI>);
////
bool flag_plan = false;
///
void cloud_seg(const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud)
{
        int size_cout = 0;
        if (cloud->size()>10)
        {
           flag_plan = true;
        }else{
            flag_plan = false;
        }
        

        RectanglePoint stop_topLeft = {-0.5, 0.5};
        RectanglePoint stop_bottomRight = {0.6, -0.5};
        
        int obs_count = 0;
        for (size_t i = 0; i < cloud->size(); i++)
        {
            if (isPointInRectangle(cloud->points[i] , stop_topLeft, stop_bottomRight) && cloud->points[i].z>(-0.1) && cloud->points[i].z<3)
            {
                seg_A_C->points.push_back(cloud->points[i]); 
                obs_count++;
            }

        }

        seg_A_C->width = seg_A_C->points.size();  
        seg_A_C->height = 1;  
        seg_A_C->is_dense = true; 
    

        if (obs_count>20)
        {
            stop_flag = true;
            std::cout << "find obstacle!!! " <<std::endl;
            std::cout << "obstacle size is " << obs_count <<std::endl;

            std::stringstream ss;
            ss << size_cout;
            std::string str = ss.str();
            pcl::io::savePCDFileASCII(root_dir+"pcd/"+str+".pcd", *seg_A_C);
            size_cout++;

        }else{
            stop_flag = false;
        }
        
        seg_A_C->clear();
    
    
        // Eigen::Vector3f center;
        // center = calculateXYZCentroid(*seg_A_C);
        //double distance_obtacle =std::sqrt(center[0]*center[0] + center[1]*center[1]);
        // if ((distance_obtacle < obstacle_distance) && (center[2]< 3) && (center[2]>(-0.1)))
        // {

        // }
        // seg_A_C->clear();
        //stop_flag = false;
        
}
 





int main(int argc, char** argv)
{
    ros::init(argc, argv, "obs_get");
    ros::NodeHandle nh;

    //nh.param<double>("pose_transform/obstacle_distance", obstacle_distance, 1);

        /*** ROS subscribe initialization ***/
    ros::Subscriber sub_pcl = nh.subscribe("/livox/lidar", 200000, livox_pcl_cbk);
    ros::Subscriber sub_imu = nh.subscribe("/livox/imu", 200000, imu_cbk);
    ros::Subscriber sub_point = nh.subscribe<geometry_msgs::Point>("/teb_target_point", 10, pointCallback);
    ros::Subscriber sub_odom = nh.subscribe<nav_msgs::Odometry>("/Odometry", 100, odom_cbk);

    ros::Publisher bool_pub = nh.advertise<std_msgs::Bool>("/stop_signl", 10);
    ros::Publisher pub_obst = nh.advertise<std_msgs::Bool>("/start_plan", 10);


    ros::Rate rate(5000);
    bool status = ros::ok();
    while (status)
    {

        ros::spinOnce();


        pcl::PointCloud<pcl::PointXYZI>::Ptr obstacle_(new pcl::PointCloud<pcl::PointXYZI>);
        if(obstacle_buffer.size()>1)
        {
            for(int i=0;i<obstacle_buffer.size();i++)
            {
                *obstacle_ += *obstacle_buffer.front();
                obstacle_buffer.pop_front();
            }
            if(obstacle_->size()<20)
            {
                std::cout << "no obstacle!!!"<<std::endl;
                stop_flag = false;
                flag_plan = false;
            }else{
                cloud_seg(obstacle_);
            }


        }

        publish_stop(bool_pub);
        
        std_msgs::Bool msg;
        msg.data = flag_plan;  // 或者设置为fals
        pub_obst.publish(msg);
        rate.sleep();
    }




    return 0;
}
