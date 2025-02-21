#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <ros/ros.h>
#include <std_msgs/String.h>

using json = nlohmann::json;

struct PIDParams {
    std::string name;
    double P;
    double I;
    double D;
};

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

int main(int argc, char** argv) {

    ros::init(argc, argv, "read_json_pid_node");
    ros::NodeHandle nh;

    PIDParams controller1;
    PIDParams controller2;

    if (readPIDParams("/home/agile/control_ws/src/bunker_control/src/pid_params.json", controller1, controller2)) {
        // 输出读取到的PID参数
        std::cout << "Controller 1 Name: " << controller1.name << std::endl;
        std::cout << "P: " << controller1.P << std::endl;
        std::cout << "I: " << controller1.I << std::endl;
        std::cout << "D: " << controller1.D << std::endl;

        std::cout << "Controller 2 Name: " << controller2.name << std::endl;
        std::cout << "P: " << controller2.P << std::endl;
        std::cout << "I: " << controller2.I << std::endl;
        std::cout << "D: " << controller2.D << std::endl;
    } else {
        std::cerr << "Failed to read PID parameters from JSON file" << std::endl;
        return 1;
    }

    return 0;
}