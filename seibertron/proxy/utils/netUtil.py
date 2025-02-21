import pywifi
import time
import yaml
import psutil
import os
import socket

import config
NETPLAN_FILE = config.NETPLAN_FILE


def wifi_scan():
    # 创建一个pywifi对象
    wifi = pywifi.PyWiFi()
    # 选择一个无线网卡，这里选择了第一个接口
    ifaces = wifi.interfaces()[0]

    status = ifaces.status()
    # 开始扫描
    ifaces.scan()
    time.sleep(5)  # 等待扫描完成，这里设置了10秒
    # 获取扫描结果
    scan_results = ifaces.scan_results()
    wifi_array = []
    for result in scan_results:
        if result.ssid not in wifi_array:
            wifi_array.append(result.ssid)
    return wifi_array

def wifi_connect(ssid, password):
    wifi = pywifi.PyWiFi()
    ifaces = wifi.interfaces()[0]
    ifaces.connect({'ssid': ssid, 'password': password})
    time.sleep(5)
    return ifaces.status()


def network_config():
    # 获取所有网络接口的信息
    net_interfaces = psutil.net_if_addrs()
    network_array = []
    # 遍历每个网络接口的信息
    for interface_name, interface_addresses in net_interfaces.items():
        for addr in interface_addresses:
            if addr.netmask is not None:
                network_array.append({"interface": interface_name,"address": addr.address, "netmask": addr.netmask })
    return network_array

def read_netplan_config():
    """读取netplan配置文件并返回其内容"""
    if not os.path.exists(NETPLAN_FILE):
        raise FileNotFoundError(f"{NETPLAN_FILE} 文件不存在")

    with open(NETPLAN_FILE, 'r') as file:
        config = yaml.safe_load(file)
    return config


def write_netplan_config(config):
    """将配置写回netplan配置文件"""
    with open(NETPLAN_FILE, 'w') as file:
        yaml.safe_dump(config, file, default_flow_style=False)


def modify_network_config(interface, new_config, type):
    """修改指定接口的网络配置"""
    config = read_netplan_config()

    if 'network' not in config:
        config['network'] = {}

    if type == 'ethernets':

        if 'ethernets' not in config['network']:
            config['network']['ethernets'] = {}

        config['network']['ethernets'][interface] = new_config
    if type == 'wifis':
        if 'wifis' not in config['network']:
            config['network']['wifis'] = {}
        config['network']['wifis'][interface] = new_config

    write_netplan_config(config)


def get_network_info():
    network_info = {}

    # 获取网卡信息
    interfaces = psutil.net_if_addrs()
    for interface, addrs in interfaces.items():
        network_info[interface] = {}
        network_info[interface]['interface'] = interface
        for addr in addrs:
            if addr.family == socket.AF_INET:  # IPv4地址
                network_info[interface]['IP Address'] = addr.address
                network_info[interface]['Subnet Mask'] = addr.netmask
            elif addr.family == socket.AF_INET6:  # IPv6地址
                network_info[interface]['IPv6 Address'] = addr.address
            elif addr.family == psutil.AF_LINK:  # MAC地址
                network_info[interface]['MAC Address'] = addr.address

    # 获取网关信息
    #gateways = psutil.net_if_stats()
    #for interface, stats in gateways.items():
    #    network_info[interface]['Is Up'] = stats.isup

    # 获取DNS信息
    #dns_info = psutil.net_if_stats()
    #for interface, stats in dns_info.items():
        #network_info[interface]['Is Up'] = stats.isup

        # Example DNS获取数据的打印
    return network_info
