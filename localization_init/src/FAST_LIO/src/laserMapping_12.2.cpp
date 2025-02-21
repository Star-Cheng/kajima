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

#define INIT_TIME           (0.1)
#define LASER_POINT_COV     (0.001)
#define MAXN                (720000)
#define PUBFRAME_PERIOD     (20)

/*** Time Log Variables ***/
double kdtree_incremental_time = 0.0, kdtree_search_time = 0.0, kdtree_delete_time = 0.0;
double T1[MAXN], s_plot[MAXN], s_plot2[MAXN], s_plot3[MAXN], s_plot4[MAXN], s_plot5[MAXN], s_plot6[MAXN], s_plot7[MAXN], s_plot8[MAXN], s_plot9[MAXN], s_plot10[MAXN], s_plot11[MAXN];
double match_time = 0, solve_time = 0, solve_const_H_time = 0;
int    kdtree_size_st = 0, kdtree_size_end = 0, add_point_size = 0, kdtree_delete_counter = 0;
bool   runtime_pos_log = false, pcd_save_en = false, time_sync_en = false, extrinsic_est_en = true, path_en = true;
/**************************/

float res_last[100000] = {0.0};
float DET_RANGE = 300.0f;
const float MOV_THRESHOLD = 1.5f;
double time_diff_lidar_to_imu = 0.0;

mutex mtx_buffer;
condition_variable sig_buffer;

string root_dir = ROOT_DIR;
string map_file_path, lid_topic, imu_topic , json_path;
string map_input_path;

double res_mean_last = 0.05, total_residual = 0.0;
double last_timestamp_lidar = 0, last_timestamp_imu = -1.0;
double gyr_cov = 0.1, acc_cov = 0.1, b_gyr_cov = 0.0001, b_acc_cov = 0.0001;
double filter_size_corner_min = 0, filter_size_surf_min = 0, filter_size_map_min = 0, fov_deg = 0;
double cube_len = 0, HALF_FOV_COS = 0, FOV_DEG = 0, total_distance = 0, lidar_end_time = 0, first_lidar_time = 0.0;
int    effct_feat_num = 0, time_log_counter = 0, scan_count = 0, publish_count = 0;
int    iterCount = 0, feats_down_size = 0, NUM_MAX_ITERATIONS = 0, laserCloudValidNum = 0, pcd_save_interval = -1, pcd_index = 0;
bool   point_selected_surf[100000] = {0};
bool   lidar_pushed, flg_first_scan = true, flg_exit = false, flg_EKF_inited;
bool   scan_pub_en = false, dense_pub_en = false, scan_body_pub_en = false;

vector<vector<int>>  pointSearchInd_surf; 
vector<BoxPointType> cub_needrm;
vector<PointVector>  Nearest_Points; 
vector<bool> add_flag;
vector<double>       extrinT(3, 0.0);
vector<double>       extrinR(9, 0.0);
deque<double>                     time_buffer;

deque<PointCloudXYZI::Ptr>        lidar_buffer;

deque<pcl::PointCloud<pcl::PointXYZI>::Ptr>    obstacle_buffer;

deque<sensor_msgs::Imu::ConstPtr> imu_buffer;

PointCloudXYZI::Ptr featsFromMap(new PointCloudXYZI());
PointCloudXYZI::Ptr feats_undistort(new PointCloudXYZI());
PointCloudXYZI::Ptr feats_down_body(new PointCloudXYZI());
PointCloudXYZI::Ptr feats_down_world(new PointCloudXYZI());
PointCloudXYZI::Ptr normvec(new PointCloudXYZI(100000, 1));
PointCloudXYZI::Ptr laserCloudOri(new PointCloudXYZI(100000, 1));
PointCloudXYZI::Ptr corr_normvect(new PointCloudXYZI(100000, 1));
PointCloudXYZI::Ptr _featsArray;

PointCloudXYZI::Ptr relocal_map(new PointCloudXYZI());


pcl::VoxelGrid<PointType> downSizeFilterSurf;
pcl::VoxelGrid<PointType> downSizeFilterMap;

KD_TREE<PointType> ikdtree;

V3F XAxisPoint_body(LIDAR_SP_LEN, 0.0, 0.0);
V3F XAxisPoint_world(LIDAR_SP_LEN, 0.0, 0.0);
V3D euler_cur;
V3D position_last(Zero3d);
V3D Lidar_T_wrt_IMU(Zero3d);
M3D Lidar_R_wrt_IMU(Eye3d);

/*** EKF inputs and output ***/
MeasureGroup Measures;
esekfom::esekf<state_ikfom, 12, input_ikfom> kf;
state_ikfom state_point;
vect3 pos_lid;

nav_msgs::Path path;
nav_msgs::Odometry odomAftMapped;
nav_msgs::Odometry tran_odom;
geometry_msgs::Quaternion geoQuat;
geometry_msgs::PoseStamped msg_body_pose;

shared_ptr<Preprocess> p_pre(new Preprocess());
shared_ptr<ImuProcess> p_imu(new ImuProcess());


// 定义一个结构体来存储目标点
struct Point {
    double x;
    double y;
    std::string tag;  // 新增tag字段，用于区分普通航迹点和任务点
};

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


void publishPath(const std::vector<Point>& positions, ros::Publisher& marker_pub) {
    visualization_msgs::Marker marker;
    marker.header.frame_id = "camera_init";
    marker.header.stamp = ros::Time::now();
    marker.ns = "update_path";
    marker.id = 0;
    marker.type = visualization_msgs::Marker::LINE_STRIP;
    marker.action = visualization_msgs::Marker::ADD;
    marker.pose.orientation.w = 1.0;

    // 设置颜色和大小
    marker.color.r = 0.0f;
    marker.color.g = 1.0f;
    marker.color.b = 0.0f;
    marker.color.a = 1.0;
    marker.scale.x = 0.1;

    for (const auto& pos : positions) {
        geometry_msgs::Point point;
        point.x = pos.x;
        point.y = pos.y;
        point.z = 1;
        marker.points.push_back(point);
    }

    marker_pub.publish(marker);
}



void SigHandle(int sig)
{
    flg_exit = true;
    ROS_WARN("catch sig %d", sig);
    sig_buffer.notify_all();
}

inline void dump_lio_state_to_log(FILE *fp)  
{
    V3D rot_ang(Log(state_point.rot.toRotationMatrix()));
    fprintf(fp, "%lf ", Measures.lidar_beg_time - first_lidar_time);
    fprintf(fp, "%lf %lf %lf ", rot_ang(0), rot_ang(1), rot_ang(2));                   // Angle
    fprintf(fp, "%lf %lf %lf ", state_point.pos(0), state_point.pos(1), state_point.pos(2)); // Pos  
    fprintf(fp, "%lf %lf %lf ", 0.0, 0.0, 0.0);                                        // omega  
    fprintf(fp, "%lf %lf %lf ", state_point.vel(0), state_point.vel(1), state_point.vel(2)); // Vel  
    fprintf(fp, "%lf %lf %lf ", 0.0, 0.0, 0.0);                                        // Acc  
    fprintf(fp, "%lf %lf %lf ", state_point.bg(0), state_point.bg(1), state_point.bg(2));    // Bias_g  
    fprintf(fp, "%lf %lf %lf ", state_point.ba(0), state_point.ba(1), state_point.ba(2));    // Bias_a  
    fprintf(fp, "%lf %lf %lf ", state_point.grav[0], state_point.grav[1], state_point.grav[2]); // Bias_a  
    fprintf(fp, "\r\n");  
    fflush(fp);
}

void pointBodyToWorld_ikfom(PointType const * const pi, PointType * const po, state_ikfom &s)
{
    V3D p_body(pi->x, pi->y, pi->z);
    V3D p_global(s.rot * (s.offset_R_L_I*p_body + s.offset_T_L_I) + s.pos);

    po->x = p_global(0);
    po->y = p_global(1);
    po->z = p_global(2);
    po->intensity = pi->intensity;
}


void pointBodyToWorld(PointType const * const pi, PointType * const po)
{
    V3D p_body(pi->x, pi->y, pi->z);
    V3D p_global(state_point.rot * (state_point.offset_R_L_I*p_body + state_point.offset_T_L_I) + state_point.pos);

    po->x = p_global(0);
    po->y = p_global(1);
    po->z = p_global(2);
    po->intensity = pi->intensity;
}

template<typename T>
void pointBodyToWorld(const Matrix<T, 3, 1> &pi, Matrix<T, 3, 1> &po)
{
    V3D p_body(pi[0], pi[1], pi[2]);
    V3D p_global(state_point.rot * (state_point.offset_R_L_I*p_body + state_point.offset_T_L_I) + state_point.pos);

    po[0] = p_global(0);
    po[1] = p_global(1);
    po[2] = p_global(2);
}

void RGBpointBodyToWorld(PointType const * const pi, PointType * const po)
{
    V3D p_body(pi->x, pi->y, pi->z);
    V3D p_global(state_point.rot * (state_point.offset_R_L_I*p_body + state_point.offset_T_L_I) + state_point.pos);

    po->x = p_global(0);
    po->y = p_global(1);
    po->z = p_global(2);
    po->intensity = pi->intensity;
}

void RGBpointBodyLidarToIMU(PointType const * const pi, PointType * const po)
{
    V3D p_body_lidar(pi->x, pi->y, pi->z);
    V3D p_body_imu(state_point.offset_R_L_I*p_body_lidar + state_point.offset_T_L_I);

    po->x = p_body_imu(0);
    po->y = p_body_imu(1);
    po->z = p_body_imu(2);
    po->intensity = pi->intensity;
}

void points_cache_collect()
{
    PointVector points_history;
    ikdtree.acquire_removed_points(points_history);
    // for (int i = 0; i < points_history.size(); i++) _featsArray->push_back(points_history[i]);
}

BoxPointType LocalMap_Points;
bool Localmap_Initialized = false;
void lasermap_fov_segment()
{
    cub_needrm.clear();
    kdtree_delete_counter = 0;
    kdtree_delete_time = 0.0;    
    pointBodyToWorld(XAxisPoint_body, XAxisPoint_world);
    V3D pos_LiD = pos_lid;
    if (!Localmap_Initialized){
        for (int i = 0; i < 3; i++){
            LocalMap_Points.vertex_min[i] = pos_LiD(i) - cube_len / 2.0;
            LocalMap_Points.vertex_max[i] = pos_LiD(i) + cube_len / 2.0;
        }
        Localmap_Initialized = true;
        return;
    }
    float dist_to_map_edge[3][2];
    bool need_move = false;
    for (int i = 0; i < 3; i++){
        dist_to_map_edge[i][0] = fabs(pos_LiD(i) - LocalMap_Points.vertex_min[i]);
        dist_to_map_edge[i][1] = fabs(pos_LiD(i) - LocalMap_Points.vertex_max[i]);
        if (dist_to_map_edge[i][0] <= MOV_THRESHOLD * DET_RANGE || dist_to_map_edge[i][1] <= MOV_THRESHOLD * DET_RANGE) need_move = true;
    }
    if (!need_move) return;
    BoxPointType New_LocalMap_Points, tmp_boxpoints;
    New_LocalMap_Points = LocalMap_Points;
    float mov_dist = max((cube_len - 2.0 * MOV_THRESHOLD * DET_RANGE) * 0.5 * 0.9, double(DET_RANGE * (MOV_THRESHOLD -1)));
    for (int i = 0; i < 3; i++){
        tmp_boxpoints = LocalMap_Points;
        if (dist_to_map_edge[i][0] <= MOV_THRESHOLD * DET_RANGE){
            New_LocalMap_Points.vertex_max[i] -= mov_dist;
            New_LocalMap_Points.vertex_min[i] -= mov_dist;
            tmp_boxpoints.vertex_min[i] = LocalMap_Points.vertex_max[i] - mov_dist;
            cub_needrm.push_back(tmp_boxpoints);
        } else if (dist_to_map_edge[i][1] <= MOV_THRESHOLD * DET_RANGE){
            New_LocalMap_Points.vertex_max[i] += mov_dist;
            New_LocalMap_Points.vertex_min[i] += mov_dist;
            tmp_boxpoints.vertex_max[i] = LocalMap_Points.vertex_min[i] + mov_dist;
            cub_needrm.push_back(tmp_boxpoints);
        }
    }
    LocalMap_Points = New_LocalMap_Points;

    points_cache_collect();
    double delete_begin = omp_get_wtime();
    if(cub_needrm.size() > 0) kdtree_delete_counter = ikdtree.Delete_Point_Boxes(cub_needrm);
    kdtree_delete_time = omp_get_wtime() - delete_begin;
}


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
bool isPointInRectangle(const PointType& point, 
                         const RectanglePoint& topLeft, 
                         const RectanglePoint& bottomRight) {
    // 检查点的x坐标是否在矩形的左边和右边之间
    bool withinX = point.x >= topLeft.x && point.x <= bottomRight.x;
    // 检查点的y坐标是否在矩形的顶部和底部之间
    bool withinY = point.y >= bottomRight.y && point.y <= topLeft.y;

    // 如果点的x和y坐标都在矩形的边界内，则点在矩形内
    return withinX && withinY;
}

//////
void standard_pcl_cbk(const sensor_msgs::PointCloud2::ConstPtr &msg) 
{
    mtx_buffer.lock();
    scan_count ++;
    double preprocess_start_time = omp_get_wtime();
    if (msg->header.stamp.toSec() < last_timestamp_lidar)
    {
        ROS_ERROR("lidar loop back, clear buffer");
        lidar_buffer.clear();
    }

    PointCloudXYZI::Ptr  ptr(new PointCloudXYZI());
    p_pre->process(msg, ptr);
    lidar_buffer.push_back(ptr);
    time_buffer.push_back(msg->header.stamp.toSec());
    last_timestamp_lidar = msg->header.stamp.toSec();
    s_plot11[scan_count] = omp_get_wtime() - preprocess_start_time;
    mtx_buffer.unlock();
    sig_buffer.notify_all();
}

double timediff_lidar_wrt_imu = 0.0;
bool   timediff_set_flg = false;

void livox_pcl_cbk(const livox_ros_driver2::CustomMsg::ConstPtr &msg) 
{
    if (acc_grav())
    {
        mtx_buffer.lock();
        double preprocess_start_time = omp_get_wtime();
        scan_count ++;
        if (msg->header.stamp.toSec() < last_timestamp_lidar)
        {
            ROS_ERROR("lidar loop back, clear buffer");
            lidar_buffer.clear();
        }
        last_timestamp_lidar = msg->header.stamp.toSec();
        
        if (!time_sync_en && abs(last_timestamp_imu - last_timestamp_lidar) > 10.0 && !imu_buffer.empty() && !lidar_buffer.empty() )
        {
            printf("IMU and LiDAR not Synced, IMU time: %lf, lidar header time: %lf \n",last_timestamp_imu, last_timestamp_lidar);
        }

        if (time_sync_en && !timediff_set_flg && abs(last_timestamp_lidar - last_timestamp_imu) > 1 && !imu_buffer.empty())
        {
            timediff_set_flg = true;
            timediff_lidar_wrt_imu = last_timestamp_lidar + 0.1 - last_timestamp_imu;
            printf("Self sync IMU and LiDAR, time diff is %.10lf \n", timediff_lidar_wrt_imu);
        }


        PointCloudXYZI::Ptr  ptr(new PointCloudXYZI());
        p_pre->process(msg, ptr);
        int size_init_cloud = ptr->size();
        int icout = 0;
        int icout1 = 0;
        PointCloudXYZI::Ptr laserCloud_tran_grav1(new PointCloudXYZI(size_init_cloud, 1));
        pcl::PointCloud<pcl::PointXYZI>::Ptr obstacle_cloud(new pcl::PointCloud<pcl::PointXYZI>(size_init_cloud, 1));        
        
        RectanglePoint topLeft = {-0.5, 0.5};
        RectanglePoint bottomRight = {2.0, -0.5};
        for (size_t i = 0; i < size_init_cloud; i++)
        {
            double angle = calculateAngleWithXAxis(&ptr->points[i]);
            double distance = calculateMagnitude(&ptr->points[i]);
            
            if (abs(angle)>150 && abs(angle)<=180 && distance < 0.5)
            {
                continue;
            }
            // if (distance < 0.2)
            // {
            //     continue;
            // }
            pointstran_grav(&ptr->points[i], \
                            &laserCloud_tran_grav1->points[icout]);
            icout++;
            if (isPointInRectangle(ptr->points[i] , topLeft, bottomRight)&&laserCloud_tran_grav1->points[i].z>(-0.3))
            {
                pointstran_grav1(&ptr->points[i], \
                            &obstacle_cloud->points[icout1]);
                icout1++;
            }
            
        }
        obstacle_cloud->points.resize(icout1);
        laserCloud_tran_grav1->points.resize(icout);

        obstacle_buffer.push_back(obstacle_cloud);
        
        if (obstacle_buffer.size()>2)
        {
            obstacle_buffer.pop_front();
        }
        

        lidar_buffer.push_back(laserCloud_tran_grav1);
        time_buffer.push_back(last_timestamp_lidar);
        
        s_plot11[scan_count] = omp_get_wtime() - preprocess_start_time;
        mtx_buffer.unlock();
        sig_buffer.notify_all();
    }
    
    
}


void imu_cbk_yuan(const sensor_msgs::Imu::ConstPtr &msg_in) 
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
         publish_count ++;
        // cout<<"IMU got at: "<<msg_in->header.stamp.toSec()<<endl;
        sensor_msgs::Imu::Ptr msg(new sensor_msgs::Imu(*msg_in));

        msg->header.stamp = ros::Time().fromSec(msg_in->header.stamp.toSec() - time_diff_lidar_to_imu);
        if (abs(timediff_lidar_wrt_imu) > 0.1 && time_sync_en)
        {
            msg->header.stamp = \
            ros::Time().fromSec(timediff_lidar_wrt_imu + msg_in->header.stamp.toSec());
        }

        double timestamp = msg->header.stamp.toSec();

        mtx_buffer.lock();

        if (timestamp < last_timestamp_imu)
        {
            ROS_WARN("imu loop back, clear buffer");
            imu_buffer.clear();
        }

        last_timestamp_imu = timestamp;
        
        imu_buffer.push_back(msg);
        mtx_buffer.unlock();
        sig_buffer.notify_all();
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
         publish_count ++;
        // cout<<"IMU got at: "<<msg_in->header.stamp.toSec()<<endl;
        sensor_msgs::Imu::Ptr msg(new sensor_msgs::Imu(*msg_in));

        msg->header.stamp = ros::Time().fromSec(msg_in->header.stamp.toSec() - time_diff_lidar_to_imu);
        if (abs(timediff_lidar_wrt_imu) > 0.1 && time_sync_en)
        {
            msg->header.stamp = \
            ros::Time().fromSec(timediff_lidar_wrt_imu + msg_in->header.stamp.toSec());
        }

        double timestamp = msg->header.stamp.toSec();

        mtx_buffer.lock();

        if (timestamp < last_timestamp_imu)
        {
            ROS_WARN("imu loop back, clear buffer");
            imu_buffer.clear();
        }

        last_timestamp_imu = timestamp;
        
        auto imu_acc = msg->linear_acceleration;
        auto gyr_acc = msg->angular_velocity;
        Eigen::Vector3f acc(imu_acc.x, imu_acc.y, imu_acc.z);
        Eigen::Vector3f gyr(gyr_acc.x, gyr_acc.y, gyr_acc.z);
        Eigen::Vector3f tran_acc(rotation_quaternion_grav*acc );
        Eigen::Vector3f tran_gyr(rotation_quaternion_grav*gyr );
        msg->linear_acceleration.x = tran_acc(0);
        msg->linear_acceleration.y = tran_acc(1);
        msg->linear_acceleration.z = tran_acc(2);
        msg->angular_velocity.x = tran_gyr(0);
        msg->angular_velocity.y = tran_gyr(1);
        msg->angular_velocity.z = tran_gyr(2);

        imu_buffer.push_back(msg);
        mtx_buffer.unlock();
        sig_buffer.notify_all();
    }
    
}

////////

bool stop_flag = false;
void publish_stop(const ros::Publisher & bool_pub)
{
    std_msgs::Bool msg;
    msg.data = true; // 或者根据需要设置为false
    bool_pub.publish(msg);

}


// 自定义比较函数，按簇大小降序排序
bool compareClusters(const pcl::PointIndices& a, const pcl::PointIndices& b) {
    return a.indices.size() > b.indices.size();
}

Eigen::Vector2f calculateXYCentroid(const pcl::PointCloud<pcl::PointXYZI>& cloud) {
    Eigen::Vector2f centroid;
    centroid.setZero(); // 初始化xy平面上的质心为零向量

    // 遍历点云，累加所有点的x和y坐标
    for (const pcl::PointXYZI& point : cloud) {
        centroid[0] += point.x; // 累加x坐标
        centroid[1] += point.y; // 累加y坐标
    }

    // 将累加的坐标除以点的数量，得到xy平面上的中心坐标
    float size = static_cast<float>(cloud.size());
    centroid[0] = centroid[0]/size;
    centroid[1] = centroid[1]/size;

    return centroid;
}
/////点云分割///
double obstacle_distance;
pcl::PointCloud<pcl::PointXYZI>::Ptr seg_A_C (new pcl::PointCloud<pcl::PointXYZI>);
void cloud_seg(const pcl::PointCloud<pcl::PointXYZI>::Ptr& cloud)
{
        
        ///聚类操作，在次去除噪点/////
        // 创建分割器对象
        pcl::search::KdTree<pcl::PointXYZI>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZI>);
        tree->setInputCloud(cloud);
        std::vector<pcl::PointIndices> cluster_indices;  
        pcl::EuclideanClusterExtraction<pcl::PointXYZI> ec;
        ec.setClusterTolerance(0.1); // 设置聚类的容差
        ec.setMinClusterSize(10); // 设置最小聚类大小
        ec.setMaxClusterSize(250000); // 设置最大聚类大小
        ec.setSearchMethod(tree);
        ec.setInputCloud(cloud);
        ec.extract(cluster_indices);


        // 从小到大排列
        //std::sort(cluster_indices.begin(), cluster_indices.end(), compareClusters);
        if (cluster_indices.size() ==0)
        {
            std::cout << "cluster fail!!"<<std::endl;
            return;
        }
        
        // 根据最大簇的索引去除小的簇
        // 提取最大的簇
        int size_cout = 0;
        for (size_t i = 0; i < cluster_indices.size(); i++)
        {
            for (std::vector<int>::const_iterator pit = cluster_indices[i].indices.begin(); pit != cluster_indices[i].indices.end(); ++pit)  
                seg_A_C->points.push_back(cloud->points[*pit]); 
            
            if (seg_A_C->points.size()<5)
            {
                seg_A_C->clear();
                continue;
            }

            seg_A_C->width = seg_A_C->points.size();  
            seg_A_C->height = 1;  
            seg_A_C->is_dense = true; 

            Eigen::Vector2f center;
            center = calculateXYCentroid(*seg_A_C);
 
            std::stringstream ss;
            // 使用<<操作符将整数写入stringstream
            ss << size_cout;
            // 使用.str()方法获取stringstream中的字符串
            std::string str = ss.str();
            pcl::io::savePCDFileASCII(root_dir+"pcd_/"+str+".pcd", *seg_A_C);
            size_cout++;

            
            double distance_obtacle =std::sqrt(center[0]*center[0] + center[1]*center[1]);
            if (distance_obtacle < obstacle_distance)
            {
                std::cout << "obstacle distance is " << distance_obtacle <<std::endl;
                std::cout << "obstacle size is " << seg_A_C->points.size() <<std::endl;
                std::cout << "cluster size is " << cluster_indices.size() <<std::endl;
                stop_flag = true;
                break;
            }
            seg_A_C->clear();
        }
        
}
 


double lidar_mean_scantime = 0.0;
int    scan_num = 0;
bool sync_packages(MeasureGroup &meas)
{
    if (lidar_buffer.empty() || imu_buffer.empty()) {
        return false;
    }

    /*** push a lidar scan ***/
    if(!lidar_pushed)
    {
        meas.lidar = lidar_buffer.front();
        meas.lidar_beg_time = time_buffer.front();
        if (meas.lidar->points.size() <= 1) // time too little
        {
            lidar_end_time = meas.lidar_beg_time + lidar_mean_scantime;
            ROS_WARN("Too few input point cloud!\n");
        }
        else if (meas.lidar->points.back().curvature / double(1000) < 0.5 * lidar_mean_scantime)
        {
            lidar_end_time = meas.lidar_beg_time + lidar_mean_scantime;
        }
        else
        {
            scan_num ++;
            lidar_end_time = meas.lidar_beg_time + meas.lidar->points.back().curvature / double(1000);
            lidar_mean_scantime += (meas.lidar->points.back().curvature / double(1000) - lidar_mean_scantime) / scan_num;
        }

        meas.lidar_end_time = lidar_end_time;

        lidar_pushed = true;
    }

    if (last_timestamp_imu < lidar_end_time)
    {
        return false;
    }

    /*** push imu data, and pop from imu buffer ***/
    double imu_time = imu_buffer.front()->header.stamp.toSec();
    meas.imu.clear();
    while ((!imu_buffer.empty()) && (imu_time < lidar_end_time))
    {
        imu_time = imu_buffer.front()->header.stamp.toSec();
        if(imu_time > lidar_end_time) break;
        meas.imu.push_back(imu_buffer.front());
        imu_buffer.pop_front();
    }

    lidar_buffer.pop_front();
    time_buffer.pop_front();
    lidar_pushed = false;
    return true;
}

int process_increments = 0;
void map_incremental(const ros::Publisher & pubLaser_addcloud)
{
    PointVector PointToAdd;
    PointVector PointNoNeedDownsample;
    PointVector PointNoNeedDownsample1;
    PointVector addmap_point;
    PointToAdd.reserve(feats_down_size);
    PointNoNeedDownsample.reserve(feats_down_size);
    PointNoNeedDownsample1.reserve(feats_down_size);
    addmap_point.reserve(feats_down_size);
    for (int i = 0; i < feats_down_size; i++)
    {
        /* transform to world frame */
        pointBodyToWorld(&(feats_down_body->points[i]), &(feats_down_world->points[i]));
        /* decide if need add to map */
        if (!Nearest_Points[i].empty() && flg_EKF_inited)
        {
            const PointVector &points_near = Nearest_Points[i];
            bool need_add = true;
            BoxPointType Box_of_Point;
            PointType downsample_result, mid_point ,init_point; 
            mid_point.x = floor(feats_down_world->points[i].x/filter_size_map_min)*filter_size_map_min + 0.5 * filter_size_map_min;
            mid_point.y = floor(feats_down_world->points[i].y/filter_size_map_min)*filter_size_map_min + 0.5 * filter_size_map_min;
            mid_point.z = floor(feats_down_world->points[i].z/filter_size_map_min)*filter_size_map_min + 0.5 * filter_size_map_min;
            float dist  = calc_dist(feats_down_world->points[i],mid_point);
            init_point.x =0;
            init_point.y =0;
            init_point.z =0;
            if (fabs(points_near[0].x - mid_point.x) > 0.5 * filter_size_map_min && fabs(points_near[0].y - mid_point.y) > 0.5 * filter_size_map_min && fabs(points_near[0].z - mid_point.z) > 0.5 * filter_size_map_min){
                PointNoNeedDownsample.push_back(feats_down_world->points[i]);
                continue;
            }
            if (calc_dist(points_near[0],feats_down_world->points[i])> filter_size_map_min && calc_dist(feats_down_body->points[i],init_point) < 20*filter_size_map_min)
            {
                PointNoNeedDownsample1.push_back(feats_down_world->points[i]);
                continue;
            }   
            
            for (int readd_i = 0; readd_i < NUM_MATCH_POINTS; readd_i ++)
            {
                if (points_near.size() < NUM_MATCH_POINTS) break;
                if (calc_dist(points_near[readd_i], mid_point) < dist)
                {
                    need_add = false;
                    break;
                }
            }
            if (need_add) PointToAdd.push_back(feats_down_world->points[i]);
        }
        else
        {
            PointToAdd.push_back(feats_down_world->points[i]);
        }
    }

    double st_time = omp_get_wtime();
    add_point_size = ikdtree.Add_Points(PointToAdd, true);
    //ikdtree.Add_Points(PointNoNeedDownsample, false); 
    add_point_size = PointToAdd.size() + PointNoNeedDownsample.size();
    int size1 = PointNoNeedDownsample1.size(); 
    int size2 = PointToAdd.size();
    //cout << size1 << "    " << size2 << endl;
    int size_add = size1 + size2;
    /////发布点云/////////
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr laseraddcloud(new pcl::PointCloud<pcl::PointXYZRGB>(size1, 1));

    // 设置点云中的所有点为红色
    for (int i = 0; i < size1; ++i)
    {
        laseraddcloud->points[i].x = PointNoNeedDownsample1[i].x; // X坐标，根据需要设置
        laseraddcloud->points[i].y = PointNoNeedDownsample1[i].y; // Y坐标，根据需要设置
        laseraddcloud->points[i].z = PointNoNeedDownsample1[i].z; // Z坐标，根据需要设置
        laseraddcloud->points[i].r = 255;   // R分量，红色
        laseraddcloud->points[i].g = 0;     // G分量，绿色
        laseraddcloud->points[i].b = 0;     // B分量，蓝色
    }

        // 设置点云中的所有点为红色
    // for (int i = size1; i < size_add; ++i)
    // {
    //     laseraddcloud->points[i].x = PointToAdd[i].x; // X坐标，根据需要设置
    //     laseraddcloud->points[i].y = PointToAdd[i].y; // Y坐标，根据需要设置
    //     laseraddcloud->points[i].z = PointToAdd[i].z; // Z坐标，根据需要设置
    //     laseraddcloud->points[i].r = 255;   // R分量，红色
    //     laseraddcloud->points[i].g = 0;     // G分量，绿色
    //     laseraddcloud->points[i].b = 0;     // B分量，蓝色
    // }

    sensor_msgs::PointCloud2 add_clouds;
    pcl::toROSMsg(*laseraddcloud, add_clouds);
    add_clouds.header.stamp = ros::Time().fromSec(lidar_end_time);
    add_clouds.header.frame_id = "camera_init";
    pubLaser_addcloud.publish(add_clouds);


    kdtree_incremental_time = omp_get_wtime() - st_time;
}

PointCloudXYZI::Ptr pcl_wait_pub(new PointCloudXYZI(500000, 1));
PointCloudXYZI::Ptr pcl_wait_save(new PointCloudXYZI());
void publish_frame_world(const ros::Publisher & pubLaserCloudFull)
{
    if(scan_pub_en)
    {
        PointCloudXYZI::Ptr laserCloudFullRes(dense_pub_en ? feats_undistort : feats_down_body);
        int size = laserCloudFullRes->points.size();
        PointCloudXYZI::Ptr laserCloudWorld( \
                        new PointCloudXYZI(size, 1));

        for (int i = 0; i < size; i++)
        {
            RGBpointBodyToWorld(&laserCloudFullRes->points[i], \
                                &laserCloudWorld->points[i]);
        }

        sensor_msgs::PointCloud2 laserCloudmsg;
        pcl::toROSMsg(*laserCloudWorld, laserCloudmsg);
        laserCloudmsg.header.stamp = ros::Time().fromSec(lidar_end_time);
        laserCloudmsg.header.frame_id = "camera_init";
        pubLaserCloudFull.publish(laserCloudmsg);
        publish_count -= PUBFRAME_PERIOD;
    }

    /**************** save map ****************/
    /* 1. make sure you have enough memories
    /* 2. noted that pcd save will influence the real-time performences **/
    if (pcd_save_en)
    {
        int size = feats_undistort->points.size();
        PointCloudXYZI::Ptr laserCloudWorld( \
                        new PointCloudXYZI(size, 1));

        for (int i = 0; i < size; i++)
        {
            RGBpointBodyToWorld(&feats_undistort->points[i], \
                                &laserCloudWorld->points[i]);
        }
        *pcl_wait_save += *laserCloudWorld;

        static int scan_wait_num = 0;
        scan_wait_num ++;
        if (pcl_wait_save->size() > 0 && pcd_save_interval > 0  && scan_wait_num >= pcd_save_interval)
        {
            pcd_index ++;
            string all_points_dir(string(string(ROOT_DIR) + "PCD/scans_") + to_string(pcd_index) + string(".pcd"));
            pcl::PCDWriter pcd_writer;
            cout << "current scan saved to /PCD/" << all_points_dir << endl;
            pcd_writer.writeBinary(all_points_dir, *pcl_wait_save);
            pcl_wait_save->clear();
            scan_wait_num = 0;
        }
    }
}

void publish_frame_body(const ros::Publisher & pubLaserCloudFull_body)
{
    int size = feats_undistort->points.size();
    PointCloudXYZI::Ptr laserCloudIMUBody(new PointCloudXYZI(size, 1));

    for (int i = 0; i < size; i++)
    {
        RGBpointBodyLidarToIMU(&feats_undistort->points[i], \
                            &laserCloudIMUBody->points[i]);
    }

    sensor_msgs::PointCloud2 laserCloudmsg;
    pcl::toROSMsg(*laserCloudIMUBody, laserCloudmsg);
    laserCloudmsg.header.stamp = ros::Time().fromSec(lidar_end_time);
    laserCloudmsg.header.frame_id = "body";
    pubLaserCloudFull_body.publish(laserCloudmsg);
    publish_count -= PUBFRAME_PERIOD;
}

void publish_effect_world(const ros::Publisher & pubLaserCloudEffect)
{
    PointCloudXYZI::Ptr laserCloudWorld( \
                    new PointCloudXYZI(effct_feat_num, 1));
    for (int i = 0; i < effct_feat_num; i++)
    {
        RGBpointBodyToWorld(&laserCloudOri->points[i], \
                            &laserCloudWorld->points[i]);
    }
    sensor_msgs::PointCloud2 laserCloudFullRes3;
    pcl::toROSMsg(*laserCloudWorld, laserCloudFullRes3);
    laserCloudFullRes3.header.stamp = ros::Time().fromSec(lidar_end_time);
    laserCloudFullRes3.header.frame_id = "camera_init";
    pubLaserCloudEffect.publish(laserCloudFullRes3);
}

void publish_map(const ros::Publisher & pubLaserCloudMap)
{
    sensor_msgs::PointCloud2 laserCloudMap;
    pcl::toROSMsg(*featsFromMap, laserCloudMap);
    laserCloudMap.header.stamp = ros::Time().fromSec(lidar_end_time);
    laserCloudMap.header.frame_id = "camera_init";
    pubLaserCloudMap.publish(laserCloudMap);
}


//////发布转换后的点云//////
void publish_tran_grav(const ros::Publisher & publish_tran_grav_cloud)
{

    if (acc_grav())
    {
        int size = feats_undistort->points.size();
        PointCloudXYZI::Ptr laserCloud_tran_world(new PointCloudXYZI(size, 1));
        PointCloudXYZI::Ptr laserCloud_tran_grav(new PointCloudXYZI(size, 1));

        for (int i = 0; i < size; i++)
        {

            RGBpointBodyToWorld(&feats_undistort->points[i], \
                                &laserCloud_tran_world->points[i]);
            pointstran_grav(&laserCloud_tran_world->points[i], \
                                &laserCloud_tran_grav->points[i]);
        }

        sensor_msgs::PointCloud2 laserCloudmsg;
        pcl::toROSMsg(*laserCloud_tran_grav, laserCloudmsg);
        laserCloudmsg.header.stamp = ros::Time().fromSec(lidar_end_time);
        laserCloudmsg.header.frame_id = "camera_init";
        publish_tran_grav_cloud.publish(laserCloudmsg);
        
        if (pcd_save_en&&0)
        {
            *pcl_wait_save += *laserCloud_tran_grav;
        }
    
    }

}



template<typename T>
void set_posestamp(T & out)
{
    out.pose.position.x = state_point.pos(0);
    out.pose.position.y = state_point.pos(1);
    out.pose.position.z = state_point.pos(2);
    out.pose.orientation.x = geoQuat.x;
    out.pose.orientation.y = geoQuat.y;
    out.pose.orientation.z = geoQuat.z;
    out.pose.orientation.w = geoQuat.w;
    
}


template<typename T>
void set_posestamp2base(T & out ,nav_msgs::Odometry & in)
{

    out.pose.orientation.x = geoQuat.w * in.pose.pose.orientation.x + geoQuat.x * in.pose.pose.orientation.w + geoQuat.y * in.pose.pose.orientation.z - geoQuat.z * in.pose.pose.orientation.y;
    out.pose.orientation.y = geoQuat.w * in.pose.pose.orientation.y - geoQuat.x * in.pose.pose.orientation.z + geoQuat.y * in.pose.pose.orientation.w + geoQuat.z * in.pose.pose.orientation.x;
    out.pose.orientation.z = geoQuat.w * in.pose.pose.orientation.z + geoQuat.x * in.pose.pose.orientation.y - geoQuat.y * in.pose.pose.orientation.x + geoQuat.z * in.pose.pose.orientation.w;
    out.pose.orientation.w = geoQuat.w * in.pose.pose.orientation.w - geoQuat.x * in.pose.pose.orientation.x - geoQuat.y * in.pose.pose.orientation.y - geoQuat.z * in.pose.pose.orientation.z;
    Eigen::Quaterniond q;
    q = Eigen::Quaterniond(in.pose.pose.orientation.w, in.pose.pose.orientation.x, in.pose.pose.orientation.y, in.pose.pose.orientation.z); // 这里假设旋转角度为0，因此四元数为纯量

    // 定义旋转矩阵
    Eigen::Matrix3d R = q.normalized().toRotationMatrix();

    // 定义三维点
    Eigen::Vector3d v(state_point.pos(0), state_point.pos(1), state_point.pos(2));

    // 应用旋转矩阵
    Eigen::Vector3d v_rotated = R * v;
    out.pose.position.x = v_rotated[0] + in.pose.pose.position.x;
    out.pose.position.y = v_rotated[1] + in.pose.pose.position.y;
    out.pose.position.z = v_rotated[2] + in.pose.pose.position.z;

}

nav_msgs::Odometry inpose;
nav_msgs::Odometry first_pose;
template<typename T>
void set_tran(T & out )
{

    out.pose.position.x = state_point.pos(0)-first_pose.pose.pose.position.x;
    out.pose.position.y = state_point.pos(1)-first_pose.pose.pose.position.y;
    out.pose.position.z = state_point.pos(2)-first_pose.pose.pose.position.z;

    Eigen::Quaterniond rotation1(first_pose.pose.pose.orientation.w, first_pose.pose.pose.orientation.x, first_pose.pose.pose.orientation.y, first_pose.pose.pose.orientation.z);
    Eigen::Quaterniond rotation2(geoQuat.w,geoQuat.x,geoQuat.y,geoQuat.z);
    Eigen::Quaterniond combined_rotation = rotation2 * rotation1;

    out.pose.orientation.x = combined_rotation.x();
    out.pose.orientation.y = combined_rotation.y();
    out.pose.orientation.z = combined_rotation.z();
    out.pose.orientation.w = combined_rotation.w();

}

void publish_odometry(const ros::Publisher & pubOdomAftMapped)
{
    odomAftMapped.header.frame_id = "camera_init";
    odomAftMapped.child_frame_id = "body";
    odomAftMapped.header.stamp = ros::Time().fromSec(lidar_end_time);// ros::Time().fromSec(lidar_end_time);
    set_posestamp(odomAftMapped.pose);
    pubOdomAftMapped.publish(odomAftMapped);
    auto P = kf.get_P();
    for (int i = 0; i < 6; i ++)
    {
        int k = i < 3 ? i + 3 : i - 3;
        odomAftMapped.pose.covariance[i*6 + 0] = P(k, 3);
        odomAftMapped.pose.covariance[i*6 + 1] = P(k, 4);
        odomAftMapped.pose.covariance[i*6 + 2] = P(k, 5);
        odomAftMapped.pose.covariance[i*6 + 3] = P(k, 0);
        odomAftMapped.pose.covariance[i*6 + 4] = P(k, 1);
        odomAftMapped.pose.covariance[i*6 + 5] = P(k, 2);
    }

    static tf::TransformBroadcaster br;
    tf::Transform                   transform;
    tf::Quaternion                  q;
    transform.setOrigin(tf::Vector3(odomAftMapped.pose.pose.position.x, \
                                    odomAftMapped.pose.pose.position.y, \
                                    odomAftMapped.pose.pose.position.z));
    q.setW(odomAftMapped.pose.pose.orientation.w);
    q.setX(odomAftMapped.pose.pose.orientation.x);
    q.setY(odomAftMapped.pose.pose.orientation.y);
    q.setZ(odomAftMapped.pose.pose.orientation.z);
    transform.setRotation( q );
    br.sendTransform( tf::StampedTransform( transform, odomAftMapped.header.stamp, "camera_init", "body" ) );
}

void publish_odometry2base(const ros::Publisher & pub_tranpose)
{
    tran_odom.header.frame_id = "base_link";
    tran_odom.child_frame_id = "body";
    tran_odom.header.stamp = ros::Time().fromSec(lidar_end_time);// ros::Time().fromSec(lidar_end_time);
    set_tran(tran_odom.pose);
    

    pub_tranpose.publish(tran_odom);
    auto P = kf.get_P();
    for (int i = 0; i < 6; i ++)
    {
        int k = i < 3 ? i + 3 : i - 3;
        tran_odom.pose.covariance[i*6 + 0] = P(k, 3);
        tran_odom.pose.covariance[i*6 + 1] = P(k, 4);
        tran_odom.pose.covariance[i*6 + 2] = P(k, 5);
        tran_odom.pose.covariance[i*6 + 3] = P(k, 0);
        tran_odom.pose.covariance[i*6 + 4] = P(k, 1);
        tran_odom.pose.covariance[i*6 + 5] = P(k, 2);
    }

    static tf::TransformBroadcaster br;
    tf::Transform                   transform;
    tf::Quaternion                  q;
    transform.setOrigin(tf::Vector3(tran_odom.pose.pose.position.x, \
                                    tran_odom.pose.pose.position.y, \
                                    tran_odom.pose.pose.position.z));
    q.setW(tran_odom.pose.pose.orientation.w);
    q.setX(tran_odom.pose.pose.orientation.x);
    q.setY(tran_odom.pose.pose.orientation.y);
    q.setZ(tran_odom.pose.pose.orientation.z);
    transform.setRotation( q );
    br.sendTransform( tf::StampedTransform( transform, tran_odom.header.stamp, "base_link", "body" ) );
}


void publish_path(const ros::Publisher pubPath)
{
    set_posestamp(msg_body_pose);
    msg_body_pose.header.stamp = ros::Time().fromSec(lidar_end_time);
    msg_body_pose.header.frame_id = "camera_init";

    /*** if path is too large, the rvis will crash ***/
    static int jjj = 0;
    jjj++;
    if (jjj % 5 == 0) 
    {
        path.poses.push_back(msg_body_pose);
        pubPath.publish(path);
    }
}

void h_share_model(state_ikfom &s, esekfom::dyn_share_datastruct<double> &ekfom_data)
{
    double match_start = omp_get_wtime();
    laserCloudOri->clear(); 
    corr_normvect->clear(); 
    total_residual = 0.0; 

    /** closest surface search and residual computation **/
    #ifdef MP_EN
        omp_set_num_threads(MP_PROC_NUM);
        #pragma omp parallel for
    #endif
    for (int i = 0; i < feats_down_size; i++)
    {
        PointType &point_body  = feats_down_body->points[i]; 
        PointType &point_world = feats_down_world->points[i]; 

        /* transform to world frame */
        V3D p_body(point_body.x, point_body.y, point_body.z);
        V3D p_global(s.rot * (s.offset_R_L_I*p_body + s.offset_T_L_I) + s.pos);
        point_world.x = p_global(0);
        point_world.y = p_global(1);
        point_world.z = p_global(2);
        point_world.intensity = point_body.intensity;

        vector<float> pointSearchSqDis(NUM_MATCH_POINTS);

        auto &points_near = Nearest_Points[i];

        if (ekfom_data.converge)
        {
            /** Find the closest surfaces in the map **/
            ikdtree.Nearest_Search(point_world, NUM_MATCH_POINTS, points_near, pointSearchSqDis);
            point_selected_surf[i] = points_near.size() < NUM_MATCH_POINTS ? false : pointSearchSqDis[NUM_MATCH_POINTS - 1] > 5 ? false : true;
        }

        if (!point_selected_surf[i]) continue;

        VF(4) pabcd;
        point_selected_surf[i] = false;
        if (esti_plane(pabcd, points_near, 0.1f))
        {
            float pd2 = pabcd(0) * point_world.x + pabcd(1) * point_world.y + pabcd(2) * point_world.z + pabcd(3);
            float s = 1 - 0.9 * fabs(pd2) / sqrt(p_body.norm());

            if (s > 0.9)
            {
                point_selected_surf[i] = true;
                normvec->points[i].x = pabcd(0);
                normvec->points[i].y = pabcd(1);
                normvec->points[i].z = pabcd(2);
                normvec->points[i].intensity = pd2;
                res_last[i] = abs(pd2);
            }
        }
    }
    
    effct_feat_num = 0;

    for (int i = 0; i < feats_down_size; i++)
    {
        if (point_selected_surf[i])
        {
            laserCloudOri->points[effct_feat_num] = feats_down_body->points[i];
            corr_normvect->points[effct_feat_num] = normvec->points[i];
            total_residual += res_last[i];
            effct_feat_num ++;
        }
    }

    if (effct_feat_num < 1)
    {
        ekfom_data.valid = false;
        ROS_WARN("No Effective Points! \n");
        return;
    }

    res_mean_last = total_residual / effct_feat_num;
    match_time  += omp_get_wtime() - match_start;
    double solve_start_  = omp_get_wtime();
    
    /*** Computation of Measuremnt Jacobian matrix H and measurents vector ***/
    ekfom_data.h_x = MatrixXd::Zero(effct_feat_num, 12); //23
    ekfom_data.h.resize(effct_feat_num);

    for (int i = 0; i < effct_feat_num; i++)
    {
        const PointType &laser_p  = laserCloudOri->points[i];
        V3D point_this_be(laser_p.x, laser_p.y, laser_p.z);
        M3D point_be_crossmat;
        point_be_crossmat << SKEW_SYM_MATRX(point_this_be);
        V3D point_this = s.offset_R_L_I * point_this_be + s.offset_T_L_I;
        M3D point_crossmat;
        point_crossmat<<SKEW_SYM_MATRX(point_this);

        /*** get the normal vector of closest surface/corner ***/
        const PointType &norm_p = corr_normvect->points[i];
        V3D norm_vec(norm_p.x, norm_p.y, norm_p.z);

        /*** calculate the Measuremnt Jacobian matrix H ***/
        V3D C(s.rot.conjugate() *norm_vec);
        V3D A(point_crossmat * C);
        if (extrinsic_est_en)
        {
            V3D B(point_be_crossmat * s.offset_R_L_I.conjugate() * C); //s.rot.conjugate()*norm_vec);
            ekfom_data.h_x.block<1, 12>(i,0) << norm_p.x, norm_p.y, norm_p.z, VEC_FROM_ARRAY(A), VEC_FROM_ARRAY(B), VEC_FROM_ARRAY(C);
        }
        else
        {
            ekfom_data.h_x.block<1, 12>(i,0) << norm_p.x, norm_p.y, norm_p.z, VEC_FROM_ARRAY(A), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
        }

        /*** Measuremnt: distance to the closest surface/corner ***/
        ekfom_data.h(i) = -norm_p.intensity;
    }
    solve_time += omp_get_wtime() - solve_start_;
}


/////////

void toEulerAngle(const Eigen::Quaternionf &q, double &roll, double &pitch, double &yaw)
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

template<typename T>
void set_oula_posestamp(T & out)
{
    Eigen::Vector3f p1(state_point.pos(0), state_point.pos(1), state_point.pos(2));
    
    Eigen::Quaternionf rotation1(geoQuat.w,geoQuat.x,geoQuat.y,geoQuat.z);


    //Eigen::Quaternionf q2 = rotation_quaternion_grav*combined_rotation;

    double r,p,y;
    toEulerAngle(rotation1,r,p,y);
    // out.pose.orientation.x = r;
    // out.pose.orientation.y = p;
    // out.pose.orientation.z = y;
    // out.pose.orientation.w = 1;

    out.pose.orientation.x = geoQuat.x;
    out.pose.orientation.y = geoQuat.y;
    out.pose.orientation.z = geoQuat.z;
    out.pose.orientation.w = geoQuat.w;

    // out.pose.position.x = rotated_p1(0)+first_pose.pose.pose.position.x - 0.08*cos(y/180*3.1415);
    // out.pose.position.y = rotated_p1(1)+first_pose.pose.pose.position.y - 0.08*sin(y/180*3.1415);
    // out.pose.position.z = rotated_p1(2)+first_pose.pose.pose.position.z;
    out.pose.position.x = p1(0) - 0.18*cos(y/180*3.1415);
    out.pose.position.y = p1(1) - 0.18*sin(y/180*3.1415);
    out.pose.position.z = p1(2);
    
}

void publish_oula_odometry(const ros::Publisher & pub_oula_odom)
{
    nav_msgs::Odometry oula_odom ;
    oula_odom.header.frame_id = "camera_init";
    oula_odom.child_frame_id = "oula_body";
    oula_odom.header.stamp = ros::Time().fromSec(lidar_end_time);// ros::Time().fromSec(lidar_end_time);
    set_oula_posestamp(oula_odom.pose);
    pub_oula_odom.publish(oula_odom);

}

//////////////////////////////



std::queue<nav_msgs::Odometry> pose_queue;
bool init_stop = false;
void odomCallback(const nav_msgs::Odometry::ConstPtr& msg)
{
    if (!init_stop)
    {
        pose_queue.push(*msg);
    }
}
/////保存txt
bool savePathToTxt(const nav_msgs::Path& path, const std::string& filename) {
    std::ofstream file;
    file.open(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    for (const auto& pose_stamped : path.poses) {
        // 假设pose_stamped包含了位置和方向信息，这里以x, y, z, roll, pitch, yaw为例
        std::stringstream pose_line;
        pose_line << pose_stamped.pose.position.x << " "
                   << pose_stamped.pose.position.y << " "
                   << pose_stamped.pose.position.z << " "; // 假设tf库用于转换四元数到欧拉角

        file << pose_line.str() << std::endl;
    }

    file.close();
    return true;
}


////
double last_save_time =0 ;
bool save_pcd = false;
void savepcd_Callback(const std_msgs::Bool::ConstPtr& msg) {
    if (msg->data) {
        // 检查自上次保存以来是否已超过3秒
        if (ros::Time::now().toSec() - last_save_time > 3.0) {
            // 保存点云到.pcd文件
            save_pcd = true;
            last_save_time = ros::Time::now().toSec();
        }
    }
}

/////

int main(int argc, char** argv)
{
    ros::init(argc, argv, "laserMapping");
    ros::NodeHandle nh;
    tf2_ros::TransformBroadcaster broadcaster;
    
    nh.param<bool>("publish/path_en",path_en, true);
    nh.param<bool>("publish/scan_publish_en",scan_pub_en, true);
    nh.param<bool>("publish/dense_publish_en",dense_pub_en, true);
    nh.param<bool>("publish/scan_bodyframe_pub_en",scan_body_pub_en, true);
    nh.param<int>("max_iteration",NUM_MAX_ITERATIONS,4);
    nh.param<string>("map_file_path",map_file_path,"");
    
    nh.param<string>("pose_transform/json_path",json_path,"");    
    
    nh.param<string>("common/lid_topic",lid_topic,"/livox/lidar");
    nh.param<string>("common/imu_topic", imu_topic,"/livox/imu");
    nh.param<bool>("common/time_sync_en", time_sync_en, false);
    nh.param<double>("common/time_offset_lidar_to_imu", time_diff_lidar_to_imu, 0.0);
    nh.param<double>("filter_size_corner",filter_size_corner_min,0.5);
    nh.param<double>("filter_size_surf",filter_size_surf_min,0.5);
    nh.param<double>("filter_size_map",filter_size_map_min,0.5);
    nh.param<double>("cube_side_length",cube_len,200);
    nh.param<float>("mapping/det_range",DET_RANGE,300.f);
    nh.param<double>("mapping/fov_degree",fov_deg,180);
    nh.param<double>("mapping/gyr_cov",gyr_cov,0.1);
    nh.param<double>("mapping/acc_cov",acc_cov,0.1);
    nh.param<double>("mapping/b_gyr_cov",b_gyr_cov,0.0001);
    nh.param<double>("mapping/b_acc_cov",b_acc_cov,0.0001);
    nh.param<double>("preprocess/blind", p_pre->blind, 0.01);
    nh.param<int>("preprocess/lidar_type", p_pre->lidar_type, AVIA);
    nh.param<int>("preprocess/scan_line", p_pre->N_SCANS, 16);
    nh.param<int>("preprocess/timestamp_unit", p_pre->time_unit, US);
    nh.param<int>("preprocess/scan_rate", p_pre->SCAN_RATE, 10);
    nh.param<int>("point_filter_num", p_pre->point_filter_num, 2);
    nh.param<bool>("feature_extract_enable", p_pre->feature_enabled, false);
    nh.param<bool>("runtime_pos_log_enable", runtime_pos_log, 0);
    nh.param<bool>("mapping/extrinsic_est_en", extrinsic_est_en, true);
    nh.param<bool>("pcd_save/pcd_save_en", pcd_save_en, false);
    nh.param<int>("pcd_save/interval", pcd_save_interval, -1);
    nh.param<vector<double>>("mapping/extrinsic_T", extrinT, vector<double>());
    nh.param<vector<double>>("mapping/extrinsic_R", extrinR, vector<double>());

    nh.param<double>("pose_transform/obstacle_distance", obstacle_distance, 1);


    bool enablePureLocalization;
    nh.param<bool>("purelocalization/enable", enablePureLocalization, false);
    std::cout << "PureLocalization:" << enablePureLocalization << std::endl;

    double x2 , y2 , z2 ,qx ,qy ,qz ,qw;
    nh.param<double>("base2cam/x2", x2, 0);
    nh.param<double>("base2cam/y2", y2, 0);
    nh.param<double>("base2cam/z2", z2, 0);
    nh.param<double>("base2cam/qx", qx, 0);
    nh.param<double>("base2cam/qy", qy, 0);
    nh.param<double>("base2cam/qz", qz, 0);
    nh.param<double>("base2cam/qw", qw, 1);
    {
        inpose.pose.pose.position.x = x2;
        inpose.pose.pose.position.y = y2;
        inpose.pose.pose.position.z = z2;
        inpose.pose.pose.orientation.w = qw;
        inpose.pose.pose.orientation.x = qx;
        inpose.pose.pose.orientation.y = qy;
        inpose.pose.pose.orientation.z = qz;

    }

    
    cout<<"p_pre->lidar_type "<<p_pre->lidar_type<<endl;
    
    path.header.stamp    = ros::Time::now();
    path.header.frame_id ="camera_init";

    /*** variables definition ***/
    int effect_feat_num = 0, frame_num = 0;
    double deltaT, deltaR, aver_time_consu = 0, aver_time_icp = 0, aver_time_match = 0, aver_time_incre = 0, aver_time_solve = 0, aver_time_const_H_time = 0;
    bool flg_EKF_converged, EKF_stop_flg = 0;
    
    FOV_DEG = (fov_deg + 10.0) > 179.9 ? 179.9 : (fov_deg + 10.0);
    HALF_FOV_COS = cos((FOV_DEG) * 0.5 * PI_M / 180.0);

    _featsArray.reset(new PointCloudXYZI());

    memset(point_selected_surf, true, sizeof(point_selected_surf));
    memset(res_last, -1000.0f, sizeof(res_last));
    downSizeFilterSurf.setLeafSize(filter_size_surf_min, filter_size_surf_min, filter_size_surf_min);
    downSizeFilterMap.setLeafSize(filter_size_map_min, filter_size_map_min, filter_size_map_min);
    memset(point_selected_surf, true, sizeof(point_selected_surf));
    memset(res_last, -1000.0f, sizeof(res_last));

    Lidar_T_wrt_IMU<<VEC_FROM_ARRAY(extrinT);
    Lidar_R_wrt_IMU<<MAT_FROM_ARRAY(extrinR);
    p_imu->set_extrinsic(Lidar_T_wrt_IMU, Lidar_R_wrt_IMU);
    p_imu->set_gyr_cov(V3D(gyr_cov, gyr_cov, gyr_cov));
    p_imu->set_acc_cov(V3D(acc_cov, acc_cov, acc_cov));
    p_imu->set_gyr_bias_cov(V3D(b_gyr_cov, b_gyr_cov, b_gyr_cov));
    p_imu->set_acc_bias_cov(V3D(b_acc_cov, b_acc_cov, b_acc_cov));

    double epsi[23] = {0.001};
    fill(epsi, epsi+23, 0.001);
    kf.init_dyn_share(get_f, df_dx, df_dw, h_share_model, NUM_MAX_ITERATIONS, epsi);

    /*** debug record ***/
    FILE *fp;
    string pos_log_dir = root_dir + "/Log/pos_log.txt";
    fp = fopen(pos_log_dir.c_str(),"w");

    ofstream fout_pre, fout_out, fout_dbg;
    fout_pre.open(DEBUG_FILE_DIR("mat_pre.txt"),ios::out);
    fout_out.open(DEBUG_FILE_DIR("mat_out.txt"),ios::out);
    fout_dbg.open(DEBUG_FILE_DIR("dbg.txt"),ios::out);
    if (fout_pre && fout_out)
        cout << "~~~~"<<ROOT_DIR<<" file opened" << endl;
    else
        cout << "~~~~"<<ROOT_DIR<<" doesn't exist" << endl;


        /*** ROS subscribe initialization ***/
    ros::Subscriber sub_pcl = p_pre->lidar_type == AVIA ? \
        nh.subscribe(lid_topic, 200000, livox_pcl_cbk) : \
        nh.subscribe(lid_topic, 200000, standard_pcl_cbk);
    ros::Subscriber sub_imu = nh.subscribe(imu_topic, 200000, imu_cbk);
    ros::Subscriber sub = nh.subscribe("/posetran", 1000, odomCallback);    

    ros::Publisher pubLaserCloudFull = nh.advertise<sensor_msgs::PointCloud2>
            ("/cloud_registered", 100000);
    ros::Publisher pubLaserCloudFull_body = nh.advertise<sensor_msgs::PointCloud2>
            ("/cloud_registered_body", 100000);
    ros::Publisher pubLaserCloudEffect = nh.advertise<sensor_msgs::PointCloud2>
            ("/cloud_effected", 100000);
    ros::Publisher pubLaserCloudMap = nh.advertise<sensor_msgs::PointCloud2>
            ("/Laser_map", 1000000);

    ros::Publisher pubLaser_addcloud = nh.advertise<sensor_msgs::PointCloud2>
            ("/add_map", 1000000);
    
    ros::Publisher bool_pub = nh.advertise<std_msgs::Bool>("stop_signl", 10);

    ros::Publisher pubOdomAftMapped = nh.advertise<nav_msgs::Odometry> 
            ("/Odometry", 100000);
    ros::Publisher pub_tranpose = nh.advertise<nav_msgs::Odometry> 
            ("/tran_Odometry", 100000);
    ros::Publisher pubPath          = nh.advertise<nav_msgs::Path> 
            ("/path", 100000);
    ros::Publisher publish_tran_grav_cloud = nh.advertise<sensor_msgs::PointCloud2>
            ("/cloud_tran_grav", 100000);
    ros::Publisher pub_oula_odom = nh.advertise<nav_msgs::Odometry> 
            ("/oula_odom", 100000);
    
    ros::Publisher pcd_pub = nh.advertise<std_msgs::Bool>("/has_save_pcd", 10);

    ros::Subscriber bool_sub = nh.subscribe("/save_pcd", 10, savepcd_Callback);
    ros::Publisher marker_pub = nh.advertise<visualization_msgs::Marker>("update_path", 10);
            


    Eigen::Vector3d init_transport;
    Eigen::Matrix3d init_R ;  
    /* read map */
    if (enablePureLocalization) {
        //double startX, startY, startZ;
        nh.param<string>("purelocalization/map_path",map_input_path, root_dir + "pcd_L/update_map.pcd");
        

        ros::Rate rate1(1000);
        while (ros::ok()) 
        {
            ros::spinOnce();
            if(pose_queue.size()>1) 
            {
                if(acc_grav())
                {
                    break;
                }
            }
            rate1.sleep();
            
        }
        if(pose_queue.size()>1) 
        {
            first_pose = pose_queue.front();
            pcl::PCDReader mapReader;
            mapReader.read(map_input_path, *relocal_map);
            // downSizeFilterMap.setInputCloud(relocal_map);
            // downSizeFilterMap.filter(*relocal_map1);
            std::cout << "22222!" << std::endl;
            
            Eigen::Vector3d translation(-first_pose.pose.pose.position.x, -first_pose.pose.pose.position.y, -first_pose.pose.pose.position.z);
            init_transport = -translation;
            // 定义旋转四元数
            Eigen::Quaterniond rotation(first_pose.pose.pose.orientation.w, first_pose.pose.pose.orientation.x, first_pose.pose.pose.orientation.y, first_pose.pose.pose.orientation.z);
            
            init_R = rotation.toRotationMatrix();  
            // 创建变换矩阵
            Eigen::Affine3d transformation = Eigen::Affine3d::Identity();
            transformation.translation() = translation;
            transformation.rotate(rotation.conjugate());
            
            // for (auto& p: relocal_map->points) {
                
            //     Eigen::Vector3d original_point(p.x, p.y, p.z);

            //     // 进行旋转和平移变换
            //     Eigen::Vector3d transformed_point = transformation * original_point;

            //     p.x = transformed_point(0);
            //     p.y = transformed_point(1);
            //     p.z = transformed_point(2);
            // }
            // pcl::io::savePCDFile( root_dir+"pcd_/tran_1.pcd", *relocal_map);
            init_stop = true;

            std::cout << "init map tran success!" << std::endl;
       
        }

        
        // pcl::VoxelGrid<pcl::PointXYZINormal> sor;
        // sor.setInputCloud(relocal_map);
        // sor.setLeafSize(0.5, 0.5, 0.5);  // 设置叶子的尺寸为5
        // sor.filter(*relocal_map1);
    }

    


//------------------------------------------------------------------------------------------------------
    signal(SIGINT, SigHandle);
    ros::Rate rate(5000);
    bool status = ros::ok();
    while (status)
    {
        if (enablePureLocalization) {
            if(ikdtree.Root_Node == nullptr)
            {

                ikdtree.Build(relocal_map->points);
                ROS_WARN("success add data");
            }
        }
        
        if (flg_exit) break;
        ros::spinOnce();
        if(sync_packages(Measures)) 
        {
            if (flg_first_scan)
            {
                first_lidar_time = Measures.lidar_beg_time;
                p_imu->first_lidar_time = first_lidar_time;
                flg_first_scan = false;
                continue;
            }

            double t0,t1,t2,t3,t4,t5,match_start, solve_start, svd_time;

            match_time = 0;
            kdtree_search_time = 0.0;
            solve_time = 0;
            solve_const_H_time = 0;
            svd_time   = 0;
            t0 = omp_get_wtime();

            p_imu->Process(Measures, kf, feats_undistort ,init_transport,init_R);
            //p_imu->Process(Measures, kf, feats_undistort);
            state_point = kf.get_x();
            pos_lid = state_point.pos + state_point.rot * state_point.offset_T_L_I;

            if (feats_undistort->empty() || (feats_undistort == NULL))
            {
                ROS_WARN("No point, skip this scan!\n");
                continue;
            }

            flg_EKF_inited = (Measures.lidar_beg_time - first_lidar_time) < INIT_TIME ? \
                            false : true;
            /*** Segment the map in lidar FOV ***/
            if (!enablePureLocalization) lasermap_fov_segment();

            /*** downsample the feature points in a scan ***/
            downSizeFilterSurf.setInputCloud(feats_undistort);
            downSizeFilterSurf.filter(*feats_down_body);
            t1 = omp_get_wtime();
            feats_down_size = feats_down_body->points.size();
            /*** initialize the map kdtree ***/
            if(!enablePureLocalization && ikdtree.Root_Node == nullptr)
            {
                if(feats_down_size > 5)
                {
                    ikdtree.set_downsample_param(filter_size_map_min);
                    feats_down_world->resize(feats_down_size);
                    for(int i = 0; i < feats_down_size; i++)
                    {
                        pointBodyToWorld(&(feats_down_body->points[i]), &(feats_down_world->points[i]));
                    }
                    ikdtree.Build(feats_down_world->points);
                }
                ROS_WARN("success add data");
                continue;
            }
            int featsFromMapNum = ikdtree.validnum();
            kdtree_size_st = ikdtree.size();
            
            // cout<<"[ mapping ]: In num: "<<feats_undistort->points.size()<<" downsamp "<<feats_down_size<<" Map num: "<<featsFromMapNum<<"effect num:"<<effct_feat_num<<endl;

            /*** ICP and iterated Kalman filter update ***/
            if (feats_down_size < 5)
            {
                ROS_WARN("No point, skip this scan!\n");
                continue;
            }
            
            normvec->resize(feats_down_size);
            feats_down_world->resize(feats_down_size);

            V3D ext_euler = SO3ToEuler(state_point.offset_R_L_I);
            fout_pre<<setw(20)<<Measures.lidar_beg_time - first_lidar_time<<" "<<euler_cur.transpose()<<" "<< state_point.pos.transpose()<<" "<<ext_euler.transpose() << " "<<state_point.offset_T_L_I.transpose()<< " " << state_point.vel.transpose() \
            <<" "<<state_point.bg.transpose()<<" "<<state_point.ba.transpose()<<" "<<state_point.grav<< endl;

            if(0) // If you need to see map point, change to "if(1)"
            {
                PointVector ().swap(ikdtree.PCL_Storage);
                ikdtree.flatten(ikdtree.Root_Node, ikdtree.PCL_Storage, NOT_RECORD);
                featsFromMap->clear();
                featsFromMap->points = ikdtree.PCL_Storage;
            }

            pointSearchInd_surf.resize(feats_down_size);
            Nearest_Points.resize(feats_down_size);
            add_flag.resize(feats_down_size);

            int  rematch_num = 0;
            bool nearest_search_en = true; //

            t2 = omp_get_wtime();
            
            /*** iterated state estimation ***/
            double t_update_start = omp_get_wtime();
            double solve_H_time = 0;
            kf.update_iterated_dyn_share_modified(LASER_POINT_COV, solve_H_time);
            state_point = kf.get_x();
            euler_cur = SO3ToEuler(state_point.rot);
            pos_lid = state_point.pos + state_point.rot * state_point.offset_T_L_I;
            geoQuat.x = state_point.rot.coeffs()[0];
            geoQuat.y = state_point.rot.coeffs()[1];
            geoQuat.z = state_point.rot.coeffs()[2];
            geoQuat.w = state_point.rot.coeffs()[3];


            geometry_msgs::TransformStamped transform;
            transform.header.frame_id = "base_link"; // 目标坐标系，例如 "world"
            transform.header.stamp = ros::Time::now();
            transform.child_frame_id = "camera_init"; // 新定义的坐标系
            transform.transform.translation.x = inpose.pose.pose.position.x;
            transform.transform.translation.y = inpose.pose.pose.position.y;
            transform.transform.translation.z = inpose.pose.pose.position.z;
            transform.transform.rotation.x = inpose.pose.pose.orientation.x;
            transform.transform.rotation.y = inpose.pose.pose.orientation.y;
            transform.transform.rotation.z = inpose.pose.pose.orientation.z;
            transform.transform.rotation.w = inpose.pose.pose.orientation.w;
                    

            broadcaster.sendTransform(transform);


            double t_update_end = omp_get_wtime();

            /******* Publish odometry *******/
            //publish_odometry2base(pub_tranpose);

            /*** add the feature points to map kdtree ***/
            t3 = omp_get_wtime();
            // if (!enablePureLocalization) 
            //map_incremental(pubLaser_addcloud);
            t5 = omp_get_wtime();
            /////避障部分
            pcl::PointCloud<pcl::PointXYZI>::Ptr obstacle_(new pcl::PointCloud<pcl::PointXYZI>);
            // if(obstacle_buffer.size()>1)
            // {
            //     for(int i=0;i<obstacle_buffer.size();i++)
            //     {
            //         *obstacle_ += *obstacle_buffer.front();
            //         obstacle_buffer.pop_front();
            //     }
            //     if(obstacle_->size()<5)
            //     {
            //         std::cout << "no obstacle!!!"<<std::endl;
            //     }else{
            //         pcl::io::savePCDFileASCII(root_dir+"pcd_/near.pcd", *obstacle_);
            //         cloud_seg(obstacle_);
            //     }


            // }

            // if (stop_flag)
            // {
            //     publish_stop(bool_pub);
            // }

            if(save_pcd)
            {
                std::time_t now = std::time(nullptr);
                std::tm* timePtr = std::localtime(&now);
                std::ostringstream fileNameStream;
                fileNameStream << std::setfill('0') << std::setw(4) << 1900 + timePtr->tm_year // 年份
                                << std::setw(2) << timePtr->tm_mon + 1 // 月份，tm_mon是从0开始的
                                << std::setw(2) << timePtr->tm_mday // 日期
                                << std::setw(2) << timePtr->tm_hour        // 小时，24小时制
                                << std::setw(2) << timePtr->tm_min   
                                << "_"; // 文件扩展名
                std::string filename_pcd = fileNameStream.str()+".pcd";
                string filename_pcd_dir(string(string(ROOT_DIR) + "pcd_/") + filename_pcd);
                
                if (pcl::io::savePCDFile(filename_pcd_dir, *pcl_wait_save) == 0) {
                    ROS_INFO("Saved point cloud to %s", filename_pcd_dir.c_str());
                    std_msgs::Bool msg;
                    msg.data = true; // 或者根据条件设置为false
                    pcd_pub.publish(msg);

                } else {
                    ROS_ERROR("Failed to save point cloud.");
                }
                save_pcd = false;
            }
                

            std::vector<Point> points = readJsonFile(json_path);
            publishPath(points, marker_pub);
            
            /******* Publish points *******/
            if (path_en)                         publish_path(pubPath);
            if (scan_pub_en || pcd_save_en)      publish_frame_world(pubLaserCloudFull);
            if (scan_pub_en && scan_body_pub_en) publish_frame_body(pubLaserCloudFull_body);
            // publish_effect_world(pubLaserCloudEffect);
            publish_map(pubLaserCloudMap);
            publish_oula_odometry(pub_oula_odom);
            publish_odometry(pubOdomAftMapped);
            //publish_tran_grav(publish_tran_grav_cloud);

            /*** Debug variables ***/
            if (runtime_pos_log)
            {
                frame_num ++;
                kdtree_size_end = ikdtree.size();
                aver_time_consu = aver_time_consu * (frame_num - 1) / frame_num + (t5 - t0) / frame_num;
                aver_time_icp = aver_time_icp * (frame_num - 1)/frame_num + (t_update_end - t_update_start) / frame_num;
                aver_time_match = aver_time_match * (frame_num - 1)/frame_num + (match_time)/frame_num;
                aver_time_incre = aver_time_incre * (frame_num - 1)/frame_num + (kdtree_incremental_time)/frame_num;
                aver_time_solve = aver_time_solve * (frame_num - 1)/frame_num + (solve_time + solve_H_time)/frame_num;
                aver_time_const_H_time = aver_time_const_H_time * (frame_num - 1)/frame_num + solve_time / frame_num;
                T1[time_log_counter] = Measures.lidar_beg_time;
                s_plot[time_log_counter] = t5 - t0;
                s_plot2[time_log_counter] = feats_undistort->points.size();
                s_plot3[time_log_counter] = kdtree_incremental_time;
                s_plot4[time_log_counter] = kdtree_search_time;
                s_plot5[time_log_counter] = kdtree_delete_counter;
                s_plot6[time_log_counter] = kdtree_delete_time;
                s_plot7[time_log_counter] = kdtree_size_st;
                s_plot8[time_log_counter] = kdtree_size_end;
                s_plot9[time_log_counter] = aver_time_consu;
                s_plot10[time_log_counter] = add_point_size;
                time_log_counter ++;
                printf("[ mapping ]: time: IMU + Map + Input Downsample: %0.6f ave match: %0.6f ave solve: %0.6f  ave ICP: %0.6f  map incre: %0.6f ave total: %0.6f icp: %0.6f construct H: %0.6f \n",t1-t0,aver_time_match,aver_time_solve,t3-t1,t5-t3,aver_time_consu,aver_time_icp, aver_time_const_H_time);
                ext_euler = SO3ToEuler(state_point.offset_R_L_I);
                fout_out << setw(20) << Measures.lidar_beg_time - first_lidar_time << " " << euler_cur.transpose() << " " << state_point.pos.transpose()<< " " << ext_euler.transpose() << " "<<state_point.offset_T_L_I.transpose()<<" "<< state_point.vel.transpose() \
                <<" "<<state_point.bg.transpose()<<" "<<state_point.ba.transpose()<<" "<<state_point.grav<<" "<<feats_undistort->points.size()<<endl;
                dump_lio_state_to_log(fp);
            }
        }

        status = ros::ok();
        rate.sleep();
    }

    /**************** save map ****************/
    /* 1. make sure you have enough memories
    /* 2. pcd save will largely influence the real-time performences **/
    
    // std::cout << "Rotation angle:\n"
    //     << rotation_angle_x << std::endl;
    // std::cout << "Rotation angle:\n"
    //     << rotation_angle_y << std::endl;
    
     // 使用ostringstream来格式化日期
    std::time_t now = std::time(nullptr);
    std::tm* timePtr = std::localtime(&now);
    std::ostringstream fileNameStream;
    fileNameStream << std::setfill('0') << std::setw(4) << 1900 + timePtr->tm_year // 年份
                    << std::setw(2) << timePtr->tm_mon + 1 // 月份，tm_mon是从0开始的
                    << std::setw(2) << timePtr->tm_mday // 日期
                    << std::setw(2) << timePtr->tm_hour        // 小时，24小时制
                    << std::setw(2) << timePtr->tm_min   
                    << "_"; // 文件扩展名
    std::string filename1 = fileNameStream.str()+".pcd";

    if (pcl_wait_save->size() > 0 && pcd_save_en)
    {

        string all_points_dir(string(string(ROOT_DIR) + "PCD/") + filename1);
        cout << "current scan saved to /PCD/" << filename1<<endl;
        pcl::PCDWriter pcd_writer;
        pcd_writer.writeBinary(all_points_dir, *pcl_wait_save);
        // if (pcl::io::savePLYFileASCII (all_points_dir, *pcl_wait_save) == -1)  
        // {  
        //     PCL_ERROR ("Couldn't write file %s \n", all_points_dir.c_str());  
        //     return (-1);  
        // }  
    }

    /////保存路径

    std::string filenametxt = "poses.txt";
    string pose_dir(string(string(ROOT_DIR) + "pose_data/") + filenametxt);
    savePathToTxt(path,pose_dir);

    fout_out.close();
    fout_pre.close();

    if (runtime_pos_log)
    {
        vector<double> t, s_vec, s_vec2, s_vec3, s_vec4, s_vec5, s_vec6, s_vec7;    
        FILE *fp2;
        string log_dir = root_dir + "/Log/fast_lio_time_log.csv";
        fp2 = fopen(log_dir.c_str(),"w");
        fprintf(fp2,"time_stamp, total time, scan point size, incremental time, search time, delete size, delete time, tree size st, tree size end, add point size, preprocess time\n");
        for (int i = 0;i<time_log_counter; i++){
            fprintf(fp2,"%0.8f,%0.8f,%d,%0.8f,%0.8f,%d,%0.8f,%d,%d,%d,%0.8f\n",T1[i],s_plot[i],int(s_plot2[i]),s_plot3[i],s_plot4[i],int(s_plot5[i]),s_plot6[i],int(s_plot7[i]),int(s_plot8[i]), int(s_plot10[i]), s_plot11[i]);
            t.push_back(T1[i]);
            s_vec.push_back(s_plot9[i]);
            s_vec2.push_back(s_plot3[i] + s_plot6[i]);
            s_vec3.push_back(s_plot4[i]);
            s_vec5.push_back(s_plot[i]);
        }
        fclose(fp2);
    }

    return 0;
}
