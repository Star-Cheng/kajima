#include <ros/ros.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "IMU_Processing.hpp"
#include "preprocess.h"
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>
#include <pcl_conversions/pcl_conversions.h> 
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_broadcaster.h>
#include <geometry_msgs/Vector3.h>
#include <ikd-Tree/ikd_Tree.h>
#include "ieskf_slam/math/geometry.h"
#include "ieskf_slam/math/SO3.h"
#include <mutex>
#include <fstream>
#include "std_msgs/Bool.h"
#include <pcl/filters/extract_indices.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/segmentation/extract_clusters.h>
#include <iostream>  
#include <vector>  
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>  
#include <CGAL/Delaunay_triangulation_2.h>  
#include <CGAL/Triangulation_vertex_base_with_info_2.h>  
#include <thread>    
#include <chrono>  
#include <std_srvs/Trigger.h>
#include <opencv2/opencv.hpp>



shared_ptr<Preprocess> p_pre(new Preprocess());
// 自定义顶点基类，包含索引信息  
struct Vertex_info {  
    size_t index; // 指向原始点云中点的索引  
};  

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;  
typedef CGAL::Triangulation_vertex_base_with_info_2<Vertex_info, K> Vb;  
typedef CGAL::Triangulation_face_base_2<K> Fb;  
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;  
typedef CGAL::Delaunay_triangulation_2<K, Tds> Delaunay;  
typedef pcl::PointXYZ PointT;  



KD_TREE<PointType> ikdtree;
//初始参数
pcl::PointCloud<PointType>::Ptr init_cloud(new pcl::PointCloud<PointType>);
pcl::PointCloud<PointType>::Ptr filteredCloud(new pcl::PointCloud<PointType>);


double x2, y2, z2, qx, qy, qz, qw; //初始位姿变化
double startx,starty,startz,startqx,startqy,startqz,startqw;
Eigen::Quaterniond rotation,strat_rotation;
Eigen::Vector3d position,strat_position;
int iter_times;
bool init_flag = false;
bool converge =true;

//转化后的地图点云
pcl::PointCloud<PointType>::Ptr tran_initcloud(new pcl::PointCloud<PointType>);

//可视化特征点云
pcl::PointCloud<pcl::PointXYZRGB>::Ptr effect_cloud(new pcl::PointCloud<pcl::PointXYZRGB>);


//定义协方差矩阵
Eigen::Matrix<double,18,18> P_init;

//收集的帧数
int frame_num = 0;

//icp匹配部分参数
template<typename _first, typename _second, typename _thrid>
struct triple{
        _first first;
        _second second;
        _thrid thrid;
};
using loss_type = triple<Eigen::Vector3d,Eigen::Vector3d,double>; //残差定义

//定义状态
struct State18
{
        Eigen::Quaterniond rotation;
        Eigen::Vector3d position;
        Eigen::Vector3d velocity;
        Eigen::Vector3d bg;
        Eigen::Vector3d ba;
        Eigen::Vector3d gravity;
        State18(){
                rotation = Eigen::Quaterniond::Identity();
                position = Eigen::Vector3d::Zero();
                velocity = Eigen::Vector3d::Zero();
                bg = Eigen::Vector3d::Zero();
                ba = Eigen::Vector3d::Zero();
                gravity = Eigen::Vector3d::Zero();
        }
};

//定义初始状态
State18 X_init;
State18 x_k_k ;

Eigen::Matrix<double,18,1> getErrorState18(const State18 &s1, const  State18 &s2){
        Eigen::Matrix<double,18,1> es;
        es.setZero();
        es.block<3,1>(0,0) = SO3Log(s2.rotation.toRotationMatrix().transpose() * s1.rotation.toRotationMatrix());
        es.block<3,1>(3,0) = s1.position - s2.position;
        es.block<3,1>(6,0) = s1.velocity - s2.velocity;
        es.block<3,1>(9,0) = s1.bg - s2.bg;
        es.block<3,1>(12,0) = s1.ba - s2.ba;
        es.block<3,1>(15,0) = s1.gravity - s2.gravity;
        return es;
}


std::mutex laser_mtx;
std::mutex odom_mtx;

std::queue<sensor_msgs::PointCloud2::ConstPtr> laser_buffer;
std::queue<nav_msgs::Odometry::ConstPtr> odom_buffer;
std::queue<nav_msgs::Odometry::ConstPtr> oulaodom_buffer;

typedef pcl::PointCloud<PointType> PointCloud;


///////接受fast_lio点云和里程计（原输出）///////用于点云叠加
// void laserCloudHandler(const sensor_msgs::PointCloud2::ConstPtr &msg) {
//   std::unique_lock<std::mutex> lock(laser_mtx);

//   laser_buffer.push(msg);
// }

void OdomHandler(const nav_msgs::Odometry::ConstPtr &msg) {
  std::unique_lock<std::mutex> lock(odom_mtx);
  odom_buffer.push(msg);
}



////////接受校正后的fast_lio里程计信息，用于无人机匹配前的定位/////
double z;
void oulaOdomHandler(const nav_msgs::Odometry::ConstPtr &msg)
{
        z = msg->pose.pose.position.z;
        oulaodom_buffer.push(msg);
}



void toEulerAngle(const Eigen::Quaterniond &q, double &roll, double &pitch, double &yaw)
{
    // roll (x-axis rotation)
    double sinr_cosp = +2.0 * (q.w() * q.x() + q.y() * q.z());
    double cosr_cosp = +1.0 - 2.0 * (q.x() * q.x() + q.y() * q.y());
    roll = atan2(sinr_cosp, cosr_cosp) * 180 / M_PI;

    double sinp = +2.0 * (q.w() * q.y() - q.z() * q.x());
    if (fabs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp) * 180 / M_PI; //
    else
        pitch = asin(sinp) * 180 / M_PI;

    // yaw (z-axis rotation)
    double siny_cosp = +2.0 * (q.w() * q.z() + q.x() * q.y());
    double cosy_cosp = +1.0 - 2.0 * (q.y() * q.y() + q.z() * q.z());
    yaw = atan2(siny_cosp, cosy_cosp) * 180 / M_PI;
    if(yaw >0)
    {
        yaw = yaw -360 ;
    }
    
}

//计算转换矩阵r，t
Eigen::Quaterniond r2; // 机体坐标系到设定pcd坐标系的转化r
Eigen::Vector3d p2; // 机体坐标系到设定pcd坐标系的转化t
void transform(const nav_msgs::Odometry& odommsg,nav_msgs::Odometry& out_odom)
{

        Eigen::Quaterniond r0(odommsg.pose.pose.orientation.w, odommsg.pose.pose.orientation.x, odommsg.pose.pose.orientation.y, odommsg.pose.pose.orientation.z);
        Eigen::Vector3d p0(-odommsg.pose.pose.position.x, odommsg.pose.pose.position.y, odommsg.pose.pose.position.z);

        // Eigen::Vector3d p00 = strat_rotation.toRotationMatrix()*(p0 - strat_position);
        // Eigen::Quaterniond r00 = r0 * strat_rotation;

        Eigen::Quaterniond r1 = X_init.rotation; // 坐标系1的姿态（四元数表示）
        Eigen::Vector3d p1 = X_init.position; // 坐标系1的位置

        // Eigen::Vector3d p11 = r1.toRotationMatrix().transpose()*(p00 - p1);
        // Eigen::Quaterniond r11 = r00 * r1;

        Eigen::Quaterniond r2 = rotation; // 坐标系2的姿态（四元数表示）
        Eigen::Vector3d p2 = position; // 坐标系2的位置

        // Eigen::Vector3d p22 = r2.toRotationMatrix() * (p11 - p2);
        // Eigen::Quaterniond r22 = r2 * r11;

        Eigen::Quaterniond r_out ; // 坐标系2的姿态（四元数表示）
        Eigen::Vector3d p_out ; // 坐标系2的位置
        
        Eigen::Vector3d p11 =r1*p0 + p1;
        Eigen::Quaterniond r11 = r1*r0;

        Eigen::Vector3d p22 =strat_rotation*p11 + strat_position;
        Eigen::Quaterniond r22 = strat_rotation*r11;

        Eigen::Vector3d p33 =r2*p22 + p2;
        Eigen::Quaterniond r33 = r2*r22;

///求起始点向对于

        r_out = r33;
        p_out = p33;

        double x1, y1, z1;
        toEulerAngle(r22, x1, y1, z1);

        // oula.header.frame_id = "base_link";
        // oula.child_frame_id = "body";

        out_odom.pose.pose.orientation.w = 100;
        out_odom.pose.pose.orientation.x = odommsg.pose.pose.orientation.x;
        out_odom.pose.pose.orientation.y = -odommsg.pose.pose.orientation.y;
        out_odom.pose.pose.orientation.z = odommsg.pose.pose.orientation.z;  //

        // 将Eigen类型的位置向量转换为geometry_msgs::Point
        out_odom.pose.pose.position.x = p_out.x();
        out_odom.pose.pose.position.y = p_out.y();
        out_odom.pose.pose.position.z = -p_out.z();

}


//////发布新的位姿//////////////
void publish_newodom(const ros::Publisher & new_odom_pub,const nav_msgs::Odometry& odommsg)
{
        nav_msgs::Odometry new_odom ;
        new_odom.header.stamp = odommsg.header.stamp;
        new_odom.child_frame_id = odommsg.child_frame_id;
        new_odom.header.frame_id = odommsg.header.frame_id;
        transform(odommsg, new_odom);
        new_odom_pub.publish(new_odom);
}

/////发布转换
void publish_tran(const ros::Publisher & posetran)
{
        nav_msgs::Odometry new_odom1 ;
        new_odom1.pose.pose.position.x = X_init.position.x()+startx;
        new_odom1.pose.pose.position.y = X_init.position.y()+starty;
        new_odom1.pose.pose.position.z = X_init.position.z()+startz;
        Eigen::Quaterniond q1(X_init.rotation.w(), X_init.rotation.x(), X_init.rotation.y(), X_init.rotation.z()); // 旋转1的四元数表示
        Eigen::Quaterniond q2(startqw, startqx, startqy, startqz);
        Eigen::Quaterniond combined_rotation = q2 * q1;

        new_odom1.pose.pose.orientation.w = combined_rotation.w();
        new_odom1.pose.pose.orientation.x = combined_rotation.x();
        new_odom1.pose.pose.orientation.y = combined_rotation.y();
        new_odom1.pose.pose.orientation.z = combined_rotation.z();  //
        posetran.publish(new_odom1);
}


//////发布初始位姿态///////////////
nav_msgs::Odometry zero_odom;
void publish_init_odom(const ros::Publisher &init_odom_pub )
{

        nav_msgs::Odometry init_odom ;
        init_odom.header.stamp = zero_odom.header.stamp;
        init_odom.child_frame_id = zero_odom.child_frame_id;
        init_odom.header.frame_id = zero_odom.header.frame_id;
        transform(zero_odom, init_odom);
        init_odom_pub.publish(init_odom);
}


///////迭代计算部分代码
bool calculate(const State18 &state,pcl::PointCloud<PointType>::Ptr &cloud,pcl::PointCloud<pcl::PointXYZRGB>::Ptr &effect_cloud, Eigen::MatrixXd & Z,Eigen::MatrixXd & H)
{
        std::vector<loss_type> loss_v;
        loss_v.resize(cloud->size());
        std::vector<bool> is_effect_point(cloud->size(),false);
        std::vector<loss_type> loss_real;
        int  vaild_points_num = 0;
        /**
         * 有效点的判断
         * 1. 将当前点变换到世界系下
         * 2. 在局部地图上使用kd_tree 临近搜索NEAR_POINTS_NUM个点
         * 3. 判断这些点是否构成平面
         * 4. 判断点离这个平面够不够近(达到阈值)
         * 5. 满足上述条件，设置为有效点。
         */
        //printf("5555555--------------------------------\n");
        for (size_t  i = 0; i < cloud->size(); i++)
        {
                // . 变换到世界系
                PointType point_init = cloud->points[i];
                PointType point_pcd , point_world;
                point_pcd = transformPoint(point_init,strat_rotation,strat_position);
                point_world = transformPoint(point_pcd,state.rotation,state.position);
                // . 临近搜索;
                // tran_initcloud->push_back(point_pcd);
                const int NEAR_POINTS_NUM = 5;
                PointVector point_ind;
                vector<float> distance(NUM_MATCH_POINTS); ////==这个地方不写括号，出现段错误！！
                
                //printf("66666--------------------------------\n");
                ikdtree.Nearest_Search(point_world,NEAR_POINTS_NUM,point_ind,distance);
                // . 是否搜索到足够的点以及最远的点到当前点的距离足够小(太远，就不认为这俩在一个平面)
                //printf("nearst point num is %d  max distance is %d\n ",distance.size(),distance[NEAR_POINTS_NUM-1]);
                //printf("nearst point num is %f\n",distance[NEAR_POINTS_NUM-1]);
                if (distance.size()<NEAR_POINTS_NUM||distance[NEAR_POINTS_NUM-1]>6)
                {
                        continue;
                }
                // . 判断这些点够不够成平面
                std::vector<PointType> planar_points;
                for (int ni = 0; ni < NEAR_POINTS_NUM; ni++)
                {
                        planar_points.push_back(point_ind[ni]);
                }
                Eigen::Vector4d pabcd;
                // . 如果构成平面

                if (planarCheck(planar_points,pabcd,0.1))    
                {
                        // . 计算点到平面距离
                        double pd = point_world.x*pabcd(0)+point_world.y*pabcd(1)+point_world.z*pabcd(2)+pabcd(3);
                        // . 记录残差
                        loss_type loss;
                        loss.thrid = pd; // 残差
                        loss.first = {point_pcd.x,point_pcd.y,point_pcd.z}; // imu系下点的坐标，用于求H
                        loss.second = pabcd.block<3,1>(0,0);// 平面法向量 用于求H
                        if (isnan(pd)||isnan(loss.second(0))||isnan(loss.second(1))||isnan(loss.second(2)))
                        {
                                printf("isnan!!\n");
                                continue;
                        }
                        // .计算点和平面的夹角，夹角越小S越大。
                        double s = 1 - 0.9 * fabs(pd) / sqrt(loss.first.norm());
                        if(s > 0.9 )
                        {
                                vaild_points_num++;
                                loss_v[i]=(loss);
                                is_effect_point[i] = true;
                        }
                }

                //printf("effect points: %d\n",vaild_points_num);

        }
        for (size_t i = 0; i <cloud->size(); i++)
        {
                if(is_effect_point[i])
                {
                        loss_real.push_back(loss_v[i]);
                        
                        pcl::PointXYZRGB point;
                        point.x = cloud->points[i].x;
                        point.y = cloud->points[i].y;
                        point.z = cloud->points[i].z;
                        point.r = 255;
                        point.g = 0;
                        point.b = 0;
                        effect_cloud->push_back(point);

                }
                
        
        }
        // 根据有效点的数量分配H Z的大小
        vaild_points_num = loss_real.size();
        H = Eigen::MatrixXd::Zero(vaild_points_num, 18); 
        Z.resize(vaild_points_num,1);
        for (int vi = 0; vi < vaild_points_num; vi++)
        {
                // H 记录导数
                Eigen::Vector3d dr = -1*loss_real[vi].second.transpose()*state.rotation.toRotationMatrix()*skewSymmetric(loss_real[vi].first);
                H.block<1,3>(vi,0) = dr.transpose();
                H.block<1,3>(vi,3) = loss_real[vi].second.transpose();
                // Z记录距离
                Z(vi,0) = loss_real[vi].thrid;
        }
        return true;
}

bool update(pcl::PointCloud<PointType>::Ptr &cloud, pcl::PointCloud<pcl::PointXYZRGB>::Ptr &effect_cloud)
{
        x_k_k = X_init;
        ///. 开迭
        Eigen::MatrixXd K;
        Eigen::MatrixXd H_k;
        Eigen::Matrix<double,18,18> P_in_update;
        int n =0;
        for (int i = 0; i < iter_times; i++)
        {
                ///. 计算误差状态 J 
                Eigen::Matrix<double,18,1> error_state = getErrorState18(x_k_k,X_init);
                printf("x_kk first: %f, second: %f\n",x_k_k.position.x(),x_k_k.position.y());
                Eigen::Matrix<double,18,18> J_inv;
                J_inv.setIdentity();            
                J_inv.block<3,3>(0,0) = A_T(error_state.block<3,1>(0,0));
                // 更新 P
                P_in_update = J_inv*P_init*J_inv.transpose();

                Eigen::MatrixXd z_k;
                //printf("444444444--------------------------------\n");
                // 调用接口计算 Z H
                calculate(x_k_k,cloud,effect_cloud,z_k,H_k);
                // printf("777777--------------------------------\n");
                Eigen::MatrixXd H_kt = H_k.transpose();
                // R 直接写死0.001; 
                K = (H_kt*H_k+(P_in_update/0.001).inverse()).inverse()*H_kt;
                //. 计算X 的增量
                Eigen::MatrixXd left = -1*K*z_k;
                Eigen::MatrixXd right = -1*(Eigen::Matrix<double,18,18>::Identity()-K*H_k)*J_inv*error_state; 
                Eigen::MatrixXd update_x = left+right;

                // 收敛判断
                n++;
                converge =true;
                for ( int idx = 0; idx < 6; idx++)
                {
                        printf("第 %d 次的 update x: %f\n", n,update_x(idx,0 )); 
                        if (update_x(idx,0)>0.001)
                        {
      
                                converge = false;
                                break;
                        }
                
                }
                // 更新X
                x_k_k.rotation = x_k_k.rotation.toRotationMatrix()*so3Exp(update_x.block<3,1>(0,0));
                x_k_k.rotation.normalize();
                x_k_k.position = x_k_k.position+update_x.block<3,1>(3,0);
                x_k_k.velocity = x_k_k.velocity+update_x.block<3,1>(6,0);
                x_k_k.bg = x_k_k.bg+update_x.block<3,1>(9,0);
                x_k_k.ba = x_k_k.ba+update_x.block<3,1>(12,0);
                x_k_k.gravity = x_k_k.gravity+update_x.block<3,1>(15,0);
                if(converge){
                        break;
                }else{
                        effect_cloud->clear();
                }
        }
        printf("iterations count is : %d\n",n);
        X_init = x_k_k;
        printf("================================\n");
        printf("T: %f ,%f ,%f \nR: %f , %f, %f, %f\n",X_init.position.x() ,X_init.position.y(),X_init.position.z(),X_init.rotation.w(),X_init.rotation.x(),X_init.rotation.y(),X_init.rotation.z());
        
        std::ofstream outfile("output.txt");
        if (!outfile.is_open()) {
                std::cerr << "Failed to open file for writing." << std::endl;
                return 1;
        }

        // 写入值到文件
        outfile << "T: " << X_init.position.x() << "," << X_init.position.y() << "," << X_init.position.z() << "\n"
                << "R: " << X_init.rotation.w() << ", " << X_init.rotation.x() << ", " << X_init.rotation.y() << ", " << X_init.rotation.z() << std::endl;

        // 关闭文件
        outfile.close();
        
        P_init = (Eigen::Matrix<double,18,18>::Identity()-K*H_k)*P_in_update;
        return converge;
}



/////停止命令
bool stored_bool_value = false;
void boolCallback(const std_msgs::Bool::ConstPtr& msg)
{
    // 存储布尔值到全局变量
    stored_bool_value = msg->data;
    // 打印接收到的布尔值
    ROS_INFO("Received bool value: %s", stored_bool_value ? "true" : "false");
}




/////点云处理////
pcl::PointCloud<PointType>::Ptr accumulatedCloud(new pcl::PointCloud<PointType>());
pcl::PointCloud<PointType>::Ptr tran_cloud(new pcl::PointCloud<PointType>());

/////livox点云
bool init_stop = true;
void pointCloudCallback(const livox_ros_driver2::CustomMsg::ConstPtr &msg) {  

    if (init_stop)
    {
        PointCloudXYZI::Ptr  ptr(new PointCloudXYZI());
        p_pre->process(msg, ptr);
        // 累积点云  
        *accumulatedCloud += *ptr; 
    }
    
}  


/////imu修正////
double x_g , x_g_r , y_g_r , z_g_r;
double y_g ;
double z_g ;
int grav_cnt = 0;
Eigen::Quaternionf rotation_quaternion_grav;

float rotation_angle_x;
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
     std::cout << "Rotation angle:\n"
              << rotation_angle_x << std::endl;

    float cos_theta_y = grav_rm_y.dot(z_axis) / (grav_rm_y.norm() * z_axis.norm());
    float sin_theta_y = std::sqrt(1.0f - cos_theta_y*cos_theta_y);
    float rotation_angle_y = std::atan2(sin_theta_y, cos_theta_y);
     std::cout << "Rotation angle:\n"
              << rotation_angle_y << std::endl;

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
        std::cout << "x_g_r: " << x_g_r << " y_g_r: " << y_g_r << "z_g_r:" <<z_g_r<<std::endl;
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

}

  
////////////////////////////////////////////////


int main(int argc, char** argv)
{
        ros::init(argc, argv,"pose_transform");
        ros::NodeHandle nh;

//   ros::Subscriber subOdom =
//       nh.subscribe<nav_msgs::Odometry>("/Odometry", 100, OdomHandler);
//   ros::Subscriber suboulaOdom =    
//       nh.subscribe<nav_msgs::Odometry>("/oula_odom", 100, oulaOdomHandler);
//   ros::Subscriber sub_grav_odom = nh.subscribe("/grav_tran", 100, grav_odomCallback);
//   ros::Subscriber sub_cloud = nh.subscribe("/cloud_registered", 1000, cloudCallback);
  
        ros::Subscriber sub = nh.subscribe("/livox/lidar", 10, pointCloudCallback);  
        ros::Subscriber sub_imu = nh.subscribe("/livox/imu", 200000, imu_cbk);
        ros::Subscriber bool_sub = nh.subscribe("/stopsignal", 10, boolCallback);
        
        ros::Publisher new_odom_pub = nh.advertise<nav_msgs::Odometry>("/new_odom", 10);
        ros::Publisher init_odom_pub =nh.advertise<nav_msgs::Odometry>("/init_pose", 10);
        ros::Publisher posetran =nh.advertise<nav_msgs::Odometry>("/posetran", 10);
        


        //读取参数
        int frame_thresholds;
        double z_thresholds;
        string pcd_path;

        nh.param<int>("pose_transform/frame_thresholds", frame_thresholds, 100);
        nh.param<double>("pose_transform/z_thresholds", z_thresholds, 10);


        nh.param<double>("pose_transform/x2", x2, 0);
        nh.param<double>("pose_transform/y2", y2, 0);
        nh.param<double>("pose_transform/z2", z2, 0);
        nh.param<string>("pose_transform/pcd_path", pcd_path, "");

        nh.param<double>("pose_transform/qx", qx, 0);
        nh.param<double>("pose_transform/qy", qy, 0);
        nh.param<double>("pose_transform/qz", qz, 0);
        nh.param<double>("pose_transform/qw", qw, 1);

        nh.param<double>("pose_transform/startx", startx, 0);
        nh.param<double>("pose_transform/starty", starty, 0);
        nh.param<double>("pose_transform/startz", startz, 0);
        nh.param<double>("pose_transform/startqx", startqx, 0);
        nh.param<double>("pose_transform/startqy", startqy, 0);
        nh.param<double>("pose_transform/startqz", startqz, 0);
        nh.param<double>("pose_transform/startqw", startqw, 1);

        nh.param<int>("pose_transform/iter_times", iter_times, 10);

        strat_rotation = Eigen::Quaterniond(startqw, startqx, startqy, startqz).conjugate();
        strat_position << startx, starty, startz;

        strat_position = -(strat_rotation*strat_position);
        rotation = Eigen::Quaterniond(qw, qx, qy, qz);
        position << x2, y2, z2;

        P_init.setIdentity();
        P_init(9,9)   = P_init(10,10) = P_init(11,11) = 0.0001;
        P_init(12,12) = P_init(13,13) = P_init(14,14) = 0.001;
        P_init(15,15) = P_init(16,16) = P_init(17,17) = 0.00001; 


        zero_odom.pose.pose.orientation.x = 0.0;
        zero_odom.pose.pose.orientation.y = 0.0;
        zero_odom.pose.pose.orientation.z = 0.0;
        zero_odom.pose.pose.orientation.w = 1.0;

        // 初始化位置部分为零向量
        zero_odom.pose.pose.position.x = 0.0;
        zero_odom.pose.pose.position.y = 0.0;
        zero_odom.pose.pose.position.z = 0.0;


        //读取pcd文件
        pcl::PointCloud<PointType>::Ptr pcd_cloud(new pcl::PointCloud<PointType>);
        if (pcl::io::loadPCDFile<PointType>(pcd_path, *pcd_cloud) == -1) 
        {
                PCL_ERROR("Couldn't read file111\n");
                return (-1);
        }

        //将点云加入到ikdtree中
        if(ikdtree.Root_Node == nullptr)
        {
                ikdtree.set_downsample_param(0.2);
                ikdtree.Build(pcd_cloud->points);
        }
        printf("1111111--------------------------------\n");
        ros::Rate rate1(10);
        ros::Time start_time = ros::Time::now(); 


        //while (z<z_thresholds)
        while (ros::ok())
        {
                ros::spinOnce();
                if ((ros::Time::now() - start_time).toSec() > 5 )
                {
                        if (acc_grav())
                        {
                                int size = accumulatedCloud->points.size();
                                PointCloudXYZI::Ptr laserCloud_tran_grav(new PointCloudXYZI(size, 1));

                                for (int i = 0; i < size; i++)
                                {

                                        pointstran_grav(&accumulatedCloud->points[i], \
                                                                &laserCloud_tran_grav->points[i]);
                                }

                                *tran_cloud = *laserCloud_tran_grav;
                        }
                        break;
                }
                
                rate1.sleep();
        }
        init_stop = false;
        printf("2222222222222--------------------------------\n");
        

        //进行体素滤波
        pcl::VoxelGrid<PointType> voxelGrid;
        voxelGrid.setInputCloud(tran_cloud);
        voxelGrid.setLeafSize(0.2, 0.2, 0.2); // 设置体素大小

        // 执行体素滤波
        voxelGrid.filter(*filteredCloud);

        pcl::io::savePCDFileASCII(string(ROOT_DIR)+"pcd_posetran/grav_tran.pcd", *filteredCloud);
        printf("save grav_tran cloud!! \n");

        printf("3333333333--------------------------------\n");
        //进行icp匹配，挑选有效点
        {
                if (update(filteredCloud ,effect_cloud))
                {

                        ROS_INFO("icp is successful!!   starting put out new odometry!!");
                        
                        init_flag = true;
                        ros::Rate rate(100);
                        while(ros::ok())
                        {
                           publish_tran(posetran);
                           rate.sleep();

                        }

                        //保存彩色有效点云

                        // while(ros::ok())
                        // {
                        //         ros::spinOnce();
                        //         nav_msgs::Odometry new_odometry;

                        //         if (!oulaodom_buffer.empty())
                        //         {
                        //                 while(oulaodom_buffer.size()>1)
                        //                 {
                        //                         oulaodom_buffer.pop();
                        //                 }
                        //                 new_odometry = *oulaodom_buffer.front();
                        //         }else
                        //         {
                        //                 ROS_WARN("odomQueue is empty!!");

                        //         }
                                
                        //         publish_newodom(new_odom_pub,new_odometry);
                        //         publish_init_odom(init_odom_pub);
                        //         rate.sleep();

                        // }
                        // pcl::VoxelGrid<PointType> voxelFilter;
                        // voxelFilter.setLeafSize(0.2f, 0.2f, 0.2f);
                        // // 执行体素滤波
                        // voxelFilter.setInputCloud(accumulatedCloud);
                        // voxelFilter.filter(*filteredCloud_cloud);

                        // cloud_seg(filteredCloud_cloud);
                        
                }
                else
                {
                        ROS_WARN("icp is not converge!!");
        
                }
                
        }

        if(converge)
        {
                for (size_t i = 0; i < filteredCloud->size(); i++)
                {
                        PointType  point_tran;
                        point_tran = transformPoint(filteredCloud->points[i],X_init.rotation,X_init.position);
                        tran_initcloud->push_back(point_tran);
                }
                
        }
        

        pcl::io::savePCDFileASCII(string(ROOT_DIR)+"pcd_posetran/map.pcd", *filteredCloud);
        printf("save map!! \n");
        pcl::io::savePCDFileASCII(string(ROOT_DIR)+"pcd_posetran/tran.pcd", *tran_initcloud);
        printf("save tran cloud!! \n");
        pcl::io::savePCDFileASCII(string(ROOT_DIR)+"pcd_posetran/effect_cloud.pcd", *effect_cloud);
        printf("save effect cloud!! \n");

        return 0;

}