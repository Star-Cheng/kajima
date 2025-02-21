# Pan


# ======================path=======================

地图： 

	map:					保存地图以及路径
	map_now/:				当前使用地图 以及 路径
	map1:					1# 地图文件夹
	map2:					2# 地图文件夹
	map3:					3# 地图文件夹


# =======================cmd=======================

mapping:  

localization:

nav:



## //设置can波特率（直接在启动文件中加载）================================

sudo apt install -y libasio-dev

	cd /home/agile/bunker_ws
	source ./devel/setup.bash
	rosrun bunker_bringup bringup_can2usb.bash    //这个是需要root权限


	#!/bin/bash
	# ----- kajima0_can.sh -----
	sudo ip link set can0 up type can bitrate 500000




## //设置激光雷达SDK(初始化)=============================================

	cd ws_livox
	source ./devel/setup.bash
	roslaunch livox_ros_driver2 msg_MID360.launch


	#!/bin/bash
	# ----- kajima1_livox.sh -----
	cd /home/agile/ws_livox
	source devel/setup.bash
	roslaunch livox_ros_driver2 msg_MID360.launch



## //设置底盘SDK（启动导航）============================================

	cd /home/agile/bunker_ws
	source ./devel/setup.bash
	roslaunch bunker_bringup bunker_robot_base.launch


	#!/bin/bash
	# ----- kajima2_bunker_sdk.sh -----
	cd /home/agile/ws_robot/bunker_ws
	source ./devel/setup.bash
	roslaunch bunker_bringup bunker_robot_base.launch



## //启动 Stereo-Pro SDK ===============================================

订阅topic  /xv_sdk/xv_dev/fisheye_cameras/left/image

	cd /home/agile/Stereo-Pro
	source devel/setup.bash
	roslaunch xv_sdk xv_sdk.launch


	#!/bin/bash
	# ----- kajima6_stereopro.sh -----
	cd /home/agile/Stereo-Pro
	source devel/setup.bash
	roslaunch xv_sdk xv_sdk.launch



## // mapping:建图 =====================================================

	cd /home/agile/fast_lp
	source ./devel/setup.bash
	roslaunch fast_lio mapping_avia.launch
	roslaunch aloam_velodyne fastlio_ouster64.launch


	#!/bin/bash
	# ----- kajima3_mapping.sh -----
	gnome-terminal --geometry=80x12+470+800 -- bash -c "cd /home/agile/ws_robot/fast_lp; source ./devel/setup.bash; roslaunch fast_lio mapping_avia.launch"
	sleep 3
	gnome-terminal --geometry=80x12+1200+800 -- bash -c "cd /home/agile/ws_robot/fast_lp; source ./devel/setup.bash; roslaunch aloam_velodyne fastlio_ouster64.launch"



## // localization:导航 ===============================================

	roslaunch fast_lio localization_mid360.launch
	rosrun fast_lio pose_tran
	rosrun fast_lio obs_get
	rosrun image_get project


	#!/bin/bash
	# ----- kajima4_localization.sh -----
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; roslaunch fast_lio localization_mid360.launch"
	sleep 3
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun fast_lio pose_tran"
	sleep 6
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun fast_lio obs_get"
	sleep 3
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/localization_init; source ./devel/setup.bash; rosrun image_get project"



## //nav: 启动控制 =====================================================

	规划所需准备 需要在规划执行之前运行
	roslaunch local_plan test.launch

	规划执行命令
	rosrun local_plan local_teb

	控制执行命令
	rosrun bunker_control demo17

	严格按照三者顺序执行即可


	#!/bin/bash
	# ----- kajima5_nav.sh -----
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/plan_ws; source ./devel/setup.bash; roslaunch local_plan test.launch"
	sleep 3
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/plan_ws; source ./devel/setup.bash; rosrun local_plan local_teb"
	sleep 5
	gnome-terminal -- bash -c "cd /home/agile/ws_robot/control_ws; source ./devel/setup.bash; rosrun bunker_control demo17"



# --------------------------------------------------------部署------------------------------------------------------------------------

## =====================bunker_ws=======================
	将launch文件中 ，  pub_tf 的值 从 true 改成 false。 
					 odom_topic_name 的default值 改成 bunker_odom 。 
	/home/aoostar/bunker_mini_ws/src/bunker_ros/bunker_bringup/launch/bunker_robot_base.launch

<?xml version="1.0"?>
<launch>

    <arg name="port_name" value="can0" />
    <arg name="simulated_robot" value="false" />
    <!--<arg name="model_xacro" default="$(find bunker_description)/urdf/bunker_v2.xacro" />-->
    <arg name="odom_topic_name" default="bunker_odom" />
    <arg name="is_bunker_mini" default="false" />
    <arg name="pub_tf" default="false" />

    <include file="$(find bunker_base)/launch/bunker_base.launch">
        <arg name="port_name" default="$(arg port_name)" />
        <arg name="simulated_robot" default="$(arg simulated_robot)" />
        <arg name="odom_topic_name" default="$(arg odom_topic_name)" />
        <arg name="pub_tf" default="$(arg pub_tf)" />
    </include>

<!--    <include file="$(find bunker_description)/launch/description.launch">
        <arg name="model_xacro" default="$(arg model_xacro)" />
    </include>-->

</launch>


## =====================control_ws=======================

	/home/agile/ws_robot/control_ws/src/bunker_control/src
	在控制路径下添加demo17.cpp 以及两个json文件

	修改control_ws/src/bunker_control/cmakelist.txt
	在cmakelist中增加以下内容
	### 添加可执行文件
	add_executable(demo17 src/demo17.cpp)

	### 将目标文件与所需的库链接
	target_link_libraries(demo17
	  ${catkin_LIBRARIES}
	  nlohmann_json::nlohmann_json  # 链接JSON库
	)

	### 安装可执行文件
	install(TARGETS demo17
	  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
	)


	在demo17.cpp中173 189 210行三处有读取路径 记得修改（应该只有主机名字有区别




	在新的规定下 路径点分为三种 normal narrow door
	例如
	[
	    {"x": 18.0, "y": -0.6, "tag": "normal"},
	    {"x": 18.19, "y": 10.76, "tag": "normal"},
	    {"x": 18.23, "y": 20.81, "tag": "door"},
	    {"x": 12.0, "y": 21.01, "tag": "narrow"},
	    {"x": 18.23, "y": 20.81, "tag": "narrow"},
	    {"x": 18.19, "y": 10.76, "tag": "narrow"},
	    {"x": 18.0, "y": -0.6, "tag": "normal"},
	    {"x": 0.0, "y": 0.0, "tag": "normal"}
	]

	normal 有避障功能 限速0.8 如需修改限速 在代码第670 671行修改
	narrow 无避障功能 限速0.5 如需修改限速 在代码第660 661行修改
	door   无避障功能 限速0.3 如需修改限速 在代码第665 666行修改



## ======================fast_lp===========================

catkin_make

	fast_lp ：  建图用。 
	fast_lp/src/FAST_LIO_LC-master/PGO/pcd/update_map.pcd                  这个路径是建好的地图(降采样地图)
	fast_lp/src/FAST_LIO_LC-master/FAST-LIO/PCD/scans.pcd                  这个路径是fastlio建图时生成的稠密点云




## ==================localization_init======================

catkin_make

	localization_init:  定位导航用
	localization_init/src/FAST_LIO/pcd_L/update_map.pcd          			这个路径下放建好的离线地图（导航用）
	localization_init/src/json/target.json                   				这个路径放的是路径点



## ========================plan============================

	sudo apt-get install ros-noetic-teb-local-planner
	sudo apt-get install ros-noetic-image-transport ros-noetic-cv-bridge
	sudo apt-get install libopencv-dev

	mkdir -p plan_ws/src
	cd plan_ws/src/
	catkin_init_workspace
	cd ..
	catkin_make
	catkin_make install
	cd src/
	catkin_create_pkg local_plan roscpp rospy std_msgs
	cd ..
	catkin_make

	规划所需准备 需要在规划执行之前运行
	roslaunch local_plan test.launch

	规划执行命令
	rosrun local_plan local_teb

	控制执行命令
	rosrun bunker_control demo17

	严格按照三者顺序执行即可


