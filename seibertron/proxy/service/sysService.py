import pywifi
import time

import psutil


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

def set_network():
    print("修改网络配置")
