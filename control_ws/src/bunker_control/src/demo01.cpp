#include <ros/ros.h>
#include <std_msgs/String.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

// 定义一个结构体来存储目标点
struct Point {
    double x;
    double y;
    double z;
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
        point.x = item.at(0).get<double>();
        point.y = item.at(1).get<double>();
        point.z = item.at(2).get<double>();
        points.push_back(point);
    }

    return points;
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "control_node");
    ros::NodeHandle nh;

    try {
        // 读取JSON文件并获取目标点
        std::string filename = "/home/agile/control_ws/src/bunker_control/src/test.json";
        std::vector<Point> points = readJsonFile(filename);

        // // 输出目标点
        // for (const auto& point : points) {
        //     ROS_INFO("Point: (%.2f, %.2f, %.2f)", point.x, point.y, point.z);
        // }
    } catch (const std::exception& e) {
        ROS_ERROR("Error: %s", e.what());
    }

    // ros::spin();
    return 0;
}