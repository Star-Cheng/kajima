#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <nav_msgs/Odometry.h>
#include <queue>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <Eigen/Dense>
#include <tf/tfMessage.h>
#include <pcl/features/normal_3d.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/segmentation/region_growing.h>
#include <pcl/filters/passthrough.h>
#include <pcl/search/kdtree.h>
#include <pcl/common/centroid.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/common/transforms.h>
#include <opencv2/opencv.hpp>
#include <pcl/segmentation/sac_segmentation.h>
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>

// 定义点云队列
std::queue<sensor_msgs::PointCloud2> point_cloud_queue;
// 定义里程计队列
std::queue<nav_msgs::Odometry> odom_queue;
// 定义队列的最大长度
const size_t MAX_FRAMES = 100;
//子地图帧数
int subnum = 10;


///////////////////////////////////////////////以下提取子地图

// 点云数据的回调函数
void cloudCallback(const sensor_msgs::PointCloud2::ConstPtr& cloud_msg) {
    point_cloud_queue.push(*cloud_msg);
    if (point_cloud_queue.size() > MAX_FRAMES) {
        point_cloud_queue.pop();
    }
}

// 里程计数据的回调函数
void odomCallback(const nav_msgs::Odometry::ConstPtr& odom_msg) {
    odom_queue.push(*odom_msg);
    if (odom_queue.size() > MAX_FRAMES) {
        odom_queue.pop();
    }
}

// 函数：将前n帧的地图叠加一起形成子地图
std::vector<pcl::PointCloud<pcl::PointXYZI>::Ptr> maps;
std::vector<nav_msgs::Odometry> odom_vector;
//判断子地图的第一帧
bool first_flag = true;
pcl::PointCloud<pcl::PointXYZI> createSubmap() 
{
    pcl::PointCloud<pcl::PointXYZI>::Ptr submap(new pcl::PointCloud<pcl::PointXYZI>);
    submap->clear();

    if (point_cloud_queue.empty() || odom_queue.empty()) {
        ROS_WARN("Point cloud or odometry queue is empty.");
        return *submap;
    }

    // 确保两个队列的长度相同
    size_t frames_to_process = std::min(point_cloud_queue.size(), odom_queue.size());

    if (subnum <= frames_to_process)
    {
            for (size_t i = 0; i < subnum; ++i) 
            {
                const sensor_msgs::PointCloud2& cloud_msg = point_cloud_queue.front();
                const nav_msgs::Odometry& odom_msg = odom_queue.front();
                double laser_timestamp = cloud_msg.header.stamp.toSec();
                double odom_timestamp = odom_msg.header.stamp.toSec();

                  // check if timestamps are matched
                if (abs(odom_timestamp - laser_timestamp) < 1e-2) 
                {
                    // 将点云转换为PCL格式
                    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);
                    pcl::fromROSMsg(cloud_msg, *cloud);

                    // 将里程计数据转换为变换矩阵
                    Eigen::Affine3f transform = Eigen::Affine3f::Identity();
                    transform.translation() << odom_msg.pose.pose.position.x, odom_msg.pose.pose.position.y, odom_msg.pose.pose.position.z;
                    transform.rotate(Eigen::Quaternionf(odom_msg.pose.pose.orientation.w, odom_msg.pose.pose.orientation.x, odom_msg.pose.pose.orientation.y, odom_msg.pose.pose.orientation.z));

                    // 应用变换矩阵到点云
                    pcl::PointCloud<pcl::PointXYZI>::Ptr transformed_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                    pcl::transformPointCloud(*cloud, *transformed_cloud, transform);

                    // 将变换后的点云添加到子地图中
                    *submap += *transformed_cloud;
                    if (first_flag)
                    {
                        odom_vector.push_back(odom_msg);
                        first_flag = false;
                    }
                    
                    // 出队
                    point_cloud_queue.pop();
                    odom_queue.pop();

                } else if (odom_timestamp < laser_timestamp) {
                    ROS_WARN("Current odometry is earlier than laser scan, discard one "
                            "odometry data.");

                    odom_queue.pop();
                    i--;

                } else {
                    ROS_WARN(
                        "Current laser scan is earlier than odometry, discard one laser scan.");

                    point_cloud_queue.pop();
                    i--;

                }

            }
            
            maps.push_back(submap);
            first_flag = true;
            return *submap;

    }else{
        return *submap;
    }

}

///////////////////////////////////////////////
///以下地面检测///////////
void estimateNormals(pcl::PointCloud<pcl::PointXYZI>::Ptr cloud, pcl::PointCloud<pcl::Normal>::Ptr normals) {
    pcl::NormalEstimation<pcl::PointXYZI, pcl::Normal> ne;
    pcl::search::KdTree<pcl::PointXYZI>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZI>());
    ne.setInputCloud(cloud);
    ne.setSearchMethod(tree);
    ne.setRadiusSearch(0.1);  // 设置搜索半径
    ne.compute(*normals);
}

// 根据法向量找出点
pcl::PointCloud<pcl::PointXYZI> filterGroundNormals(pcl::PointCloud<pcl::PointXYZI>::Ptr cloud, 
                         pcl::PointCloud<pcl::Normal>::Ptr normals,
                         pcl::PointIndices::Ptr ground_indices,
                         pcl::PointCloud<pcl::PointXYZI>::Ptr no_ground_cloud) {
    const double angle_threshold = 10.0 * M_PI / 180.0;  // 10度的角度
    const double sin_angle_threshold = std::sin(angle_threshold); 

    // 创建一个新的点云以存储点
    pcl::PointCloud<pcl::PointXYZI>::Ptr ground_cloud(new pcl::PointCloud<pcl::PointXYZI>());

    for (size_t i = 0; i < normals->size(); ++i) {
        const pcl::Normal& normal = normals->points[i];
        Eigen::Vector3f normal_vec(normal.normal_x, normal.normal_y, normal.normal_z);
        
        // 计算法向量的模长
        double norm_length = normal_vec.norm();

        // 计算 Z 分量的最大允许值
        double threshold = sin_angle_threshold * norm_length;


        // 检查法向量的 Z 分量是否接近于 0
        if ((std::abs(normal_vec[0]) < threshold)&&(std::abs(normal_vec[1]) < threshold)) {  // 保留与 XOY 平面平行的法向量
            ground_indices->indices.push_back(i);
            ground_cloud->points.push_back(cloud->points[i]);
        }else{
            no_ground_cloud->points.push_back(cloud->points[i]);
        }
    }

    // 设置点云的宽度和高度
    ground_cloud->width = ground_cloud->points.size();
    ground_cloud->height = 1;  
    ground_cloud->is_dense = true; 
    no_ground_cloud->width = no_ground_cloud->points.size();
    no_ground_cloud->height = 1;  
    no_ground_cloud->is_dense = true; 

    return *ground_cloud;

}

///地面方程
Eigen::Vector4f detect_ground(pcl::PointCloud<pcl::PointXYZI>::Ptr scan_msgs) {
    // 创建分割对象并设置参数
    pcl::SACSegmentation<pcl::PointXYZI> seg;
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.1);  // 设置距离阈值，根据实际情况调整

    // 设置输入点云
    seg.setInputCloud(scan_msgs);

    // 创建模型系数和内点索引的容器
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);

    // 执行分割
    seg.segment(*inliers, *coefficients);

    if (inliers->indices.size() == 0) {
        PCL_ERROR("Could not estimate a planar model for the given dataset.");
        return Eigen::Vector4f::Zero();  // 如果没有找到平面，返回零向量
    }

    // 模型系数包含了平面方程的参数：Ax + By + Cz + D = 0
    Eigen::Vector4f plane_params(coefficients->values[0], coefficients->values[1], coefficients->values[2], coefficients->values[3]);

    return plane_params;
}

////
void projectPointCloudToPlane_yuan(const pcl::PointCloud<pcl::PointXYZI>::Ptr cloud,
                              const Eigen::Vector4f& plane_equation,
                              cv::Mat& image) 
{
    int imageWidth = image.cols;
    int imageHeight = image.rows;

    // 创建一个用于存储点强度的矩阵，初始化为0
    cv::Mat intensityMap = cv::Mat::zeros(imageHeight, imageWidth, CV_32F);

    // 遍历点云中的每个点
    for (size_t i = 0; i < cloud->size(); ++i) {
        const pcl::PointXYZI& point = cloud->points[i];
        if (point.z>1.5||point.z<0 )
        {
            continue;
        }


        // 计算点到平面的距离
        float distance = plane_equation[0] * point.x + 
                         plane_equation[1] * point.y + 
                         plane_equation[2] * point.z - 
                         plane_equation[3];

        //投影点到平面 (这里仅示例，实际投影需要考虑实际的几何变换)
        float projected_x = point.x + distance * plane_equation[0];
        float projected_y = point.y + distance * plane_equation[1];
        // float projected_x = point.x ;
        // float projected_y = point.y ;
    

        // 将投影坐标转换为像素坐标
        int pixelX = static_cast<int>((projected_x+12)/24*imageWidth);
        int pixelY = static_cast<int>((-projected_y+10)/20*imageHeight);

        // 确保像素坐标在图像范围内
        if (pixelX >= 1 && pixelX < (imageWidth+1) && pixelY >= 1 && pixelY < (imageHeight+1)) {

                    intensityMap.at<float>(pixelY, pixelX)+=50;
                
        }

    }
    // 将强度映射到颜色上，颜色强度越大，颜色越强烈
    // 这里简化处理，仅将强度图转换为灰度图
    
    cv::normalize(intensityMap, intensityMap, 0, 255, cv::NORM_MINMAX, CV_8UC1);  

        // 创建一个与intensityMap同样大小的二值图像  

    cv::Mat binaryImage = cv::Mat::zeros(imageHeight, imageWidth, CV_8UC1);  


    cv::threshold(intensityMap, binaryImage, 10, 255, cv::THRESH_BINARY_INV);  
    //intensityMap.convertTo(image, CV_8UC1, 255.0 / intensityMap.rows);
    binaryImage.copyTo(image); 
}

void projectPointCloudToPlane1(const pcl::PointCloud<pcl::PointXYZI>::Ptr cloud,
                              const Eigen::Vector4f& plane_equation,
                              cv::Mat& image) 
{
    int imageWidth = image.cols;
    int imageHeight = image.rows;

    // 创建一个用于存储点强度的矩阵，初始化为0
    cv::Mat intensityMap = cv::Mat::zeros(imageHeight, imageWidth, CV_32F);

    // 遍历点云中的每个点
    for (size_t i = 0; i < cloud->size(); ++i) {
        const pcl::PointXYZI& point = cloud->points[i];
        if (point.z>1.5||point.z<0 )
        {
            continue;
        }


        // 计算点到平面的距离
        float distance = plane_equation[0] * point.x + 
                         plane_equation[1] * point.y + 
                         plane_equation[2] * point.z - 
                         plane_equation[3];

        //投影点到平面 (这里仅示例，实际投影需要考虑实际的几何变换)
        float projected_x = point.x + distance * plane_equation[0];
        float projected_y = point.y + distance * plane_equation[1];
        // float projected_x = point.x ;
        // float projected_y = point.y ;
    

        // 将投影坐标转换为像素坐标
        int pixelX = static_cast<int>((projected_x+5)/10*imageWidth);
        int pixelY = static_cast<int>((-projected_y+5)/10*imageHeight);

        // 确保像素坐标在图像范围内
        if (pixelX >= 1 && pixelX < (imageWidth+1) && pixelY >= 1 && pixelY < (imageHeight+1)) {

                    intensityMap.at<float>(pixelY, pixelX)+=50;
                
        }

    }
    // 将强度映射到颜色上，颜色强度越大，颜色越强烈
    // 这里简化处理，仅将强度图转换为灰度图
    
    //intensityMap.convertTo(image, CV_8UC1, 255.0 / intensityMap.rows);
    cv::normalize(intensityMap, intensityMap, 0, 255, cv::NORM_MINMAX, CV_8UC1);  

        // 创建一个与intensityMap同样大小的二值图像  

    cv::Mat binaryImage = cv::Mat::zeros(imageHeight, imageWidth, CV_8UC1);  


    cv::threshold(intensityMap, binaryImage, 0, 255, cv::THRESH_BINARY_INV);  

    //binaryImage.copyTo(image);  

    // 应用图像腐蚀  
    //cv::Mat erodedImage;  
    int erosion_size = 1; // 定义腐蚀核的大小，这里为3x3  
    cv::erode(binaryImage, image, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size)));  
    // int erosion_size = 1;
    // int iterations = 2; // 腐蚀操作的次数
    // cv::erode(binaryImage, erodedImage, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size)), cv::Point(-1, -1), iterations);
     // 将腐蚀后的图像复制到输出图像  
    //erodedImage.copyTo(image); 

}


void projectPointCloudToPlane(const pcl::PointCloud<pcl::PointXYZI>::Ptr cloud,
                              const Eigen::Vector4f& plane_equation,
                              cv::Mat& image) 
{
    int imageWidth = image.cols;
    int imageHeight = image.rows;

    // 确保image是三通道的
    if (image.channels() != 3) {
        // 如果不是三通道，可以根据需要抛出异常或者转换为三通道
        // 例如，可以使用cvtColor函数将图像转换为三通道
        throw std::runtime_error("Input image must be a 3-channel image.");
    }

    // 创建一个用于存储点强度的矩阵，初始化为0
    cv::Mat intensityMap = cv::Mat::zeros(imageHeight, imageWidth, CV_32F);

    // 遍历点云中的每个点
    for (size_t i = 0; i < cloud->size(); ++i) {
        const pcl::PointXYZI& point = cloud->points[i];
        if (point.z > 1.5 || point.z < 0) {
            continue;
        }

        // 计算点到平面的距离
        float distance = plane_equation[0] * point.x + 
                         plane_equation[1] * point.y + 
                         plane_equation[2] * point.z - 
                         plane_equation[3];

        // 投影点到平面
        float projected_x = point.x + distance * plane_equation[0];
        float projected_y = point.y + distance * plane_equation[1];

        // 将投影坐标转换为像素坐标
        int pixelX = static_cast<int>((projected_x + 5) / 10 * imageWidth);
        int pixelY = static_cast<int>((-projected_y + 5) / 10 * imageHeight);

        // 确保像素坐标在图像范围内
        if (pixelX >= 0 && pixelX < imageWidth && pixelY >= 0 && pixelY < imageHeight) {
            intensityMap.at<float>(pixelY, pixelX) += point.intensity; // 使用点的强度
        }
    }

    // 将强度映射到颜色上，颜色强度越大，颜色越强烈
    cv::normalize(intensityMap, intensityMap, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // 创建一个与intensityMap同样大小的三通道图像
    cv::Mat colorImage(imageHeight, imageWidth, CV_8UC3, cv::Scalar(0, 0, 0));

    // 将强度图转换为BGR颜色
    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x) {
            uchar intensity = intensityMap.at<uchar>(y, x);
            colorImage.at<cv::Vec3b>(y, x) = intensity > 20 ? cv::Vec3b(0, 0, 0) : cv::Vec3b(255, 255, 255);
        }
    }

    // // 创建一个与intensityMap同样大小的二值图像
    // cv::Mat binaryImage = cv::Mat::zeros(imageHeight, imageWidth, CV_8UC1);

    // cv::threshold(intensityMap, binaryImage, 0, 255, cv::THRESH_BINARY_INV);

    // // 应用图像腐蚀
    // int erosion_size = 1; // 定义腐蚀核的大小，这里为3x3
    // cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1), cv::Point(erosion_size, erosion_size));
    // cv::erode(binaryImage, colorImage, element);

    // 将腐蚀后的图像复制到输出图像
    colorImage.copyTo(image);
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "submap_creator");
    ros::NodeHandle nh;

    // 创建订阅者
    ros::Subscriber cloud_sub = nh.subscribe<sensor_msgs::PointCloud2>("/cloud_registered_body", 1000, cloudCallback);
    ros::Subscriber odom_sub = nh.subscribe<nav_msgs::Odometry>("/Odometry", 1000, odomCallback);
    //ros::Publisher pub = nh.advertise<sensor_msgs::Image>("image_topic", 1);
        // 创建ImageTransport实例
    image_transport::ImageTransport it(nh);
    // 创建发布者，发布话题名为"image_topic"
    image_transport::Publisher pub = it.advertise("/image_topic", 1);
    
    ros::Rate rate(10);
    int cnt = 1;
    std::string root_dir = ROOT_DIR_IMAGE;
    while (ros::ok())
    {
        ros::spinOnce();
        pcl::PointCloud<pcl::PointXYZI>::Ptr sub_map(new pcl::PointCloud<pcl::PointXYZI>);
        *sub_map = createSubmap();
        if (!sub_map->points.empty())
        {

                ////里程计
                nav_msgs::Odometry cur_first_odom = odom_vector.back();

                ////转点云到车身坐标
                pcl::PointCloud<pcl::PointXYZI>::Ptr tran_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                Eigen::Affine3f transform = Eigen::Affine3f::Identity();
                transform.translation() << cur_first_odom.pose.pose.position.x, cur_first_odom.pose.pose.position.y, cur_first_odom.pose.pose.position.z;
                transform.rotate(Eigen::Quaternionf(cur_first_odom.pose.pose.orientation.w, cur_first_odom.pose.pose.orientation.x, cur_first_odom.pose.pose.orientation.y, cur_first_odom.pose.pose.orientation.z));
                // 应用变换矩阵到点云
                Eigen::Affine3f inverse_transform = transform.inverse();
                pcl::transformPointCloud(*sub_map, *tran_cloud, inverse_transform);

                ////取一定范围点云
                pcl::PointCloud<pcl::PointXYZI>::Ptr near_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                for (size_t i = 0; i < tran_cloud->size(); i++)
                {
                    pcl::PointXYZI point1 = tran_cloud->points[i];
                    if (point1.x>5||point1.y>5||point1.x<(-5)||point1.y<(-5))
                    {
                            continue;
                    }
                    near_cloud->push_back(point1);
                }
                ///////////以下地面检测
                //计算法向量
                pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>());
                estimateNormals(tran_cloud, normals);
                //得到地面点
                pcl::PointIndices::Ptr ground_indices(new pcl::PointIndices());
                pcl::PointCloud<pcl::PointXYZI>::Ptr ground_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                pcl::PointCloud<pcl::PointXYZI>::Ptr no_ground_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                *ground_cloud = filterGroundNormals(tran_cloud, normals, ground_indices,no_ground_cloud);
                ///地面方程
                Eigen::Vector4f fun_ground = detect_ground(ground_cloud);
                
                // 准备一个图像对象，用于显示结果
                cv::Mat image(200, 200, CV_8UC3, cv::Scalar(0, 0, 0)); // 假设图像大小为640x480
                //cv::Mat image = cv::Mat::zeros(200, 200, CV_8UC3);  
                if (fun_ground == Eigen::Vector4f::Zero())
                {
                    fun_ground << 0, 0, 1, 0;
                    projectPointCloudToPlane(no_ground_cloud, fun_ground, image);
                }else{
                    
                    projectPointCloudToPlane(no_ground_cloud, fun_ground, image);
                }

                std::string filename = root_dir+"/image/" + std::to_string(cnt) + ".png";
                    // 保存图像到文件
                // bool isSaved = cv::imwrite(filename, image);

                // if (isSaved) {
                //     // 图片保存成功
                //     cnt++;
                //     std::cout << "Image saved successfully as " << filename << std::endl;
                // } else {
                //     // 图片保存失败
                //     std::cout << "Failed to save image as " << filename << std::endl;
                // }
                sensor_msgs::ImagePtr msg = cv_bridge::CvImage(std_msgs::Header(), "bgr8", image).toImageMsg();
                pub.publish(msg);
                std::cout << "Image saved successfully as " << filename << std::endl;



        }else{
            continue;
        }
        
        rate.sleep();
        
    }

    return 0;
}